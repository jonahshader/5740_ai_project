-- This houses the high level state machine, which is dictated
-- from what is sent from PS to PL. This does not handle sending
-- data from PL to PS. That is handled in comms_tx.

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.game_types.all;
use work.ga_types.all;

entity comms_rx is
  port (
    -- master clock, uart io
    clk           : in std_logic;
    uart_rx       : in std_logic_vector(7 downto 0);
    uart_rx_valid : in std_logic;

    -- system controls
    training_go       : out boolean       := false;
    training_pause    : out boolean       := false; -- TODO: double-check logic on this
    training_resume   : out boolean       := false; -- TODO: implement here and in c++
    inference_go      : out boolean       := false;
    inference_stop    : out boolean       := false;
    human_input       : out playerinput_t := default_playerinput_t;
    human_input_valid : out boolean       := false;
    test_go           : out boolean       := false;

    -- configs
    tilemap         : out tilemap_t   := test_tilemap_t;
    ga_config       : out ga_config_t := default_ga_config_t;
    play_against_nn : out boolean     := false -- TODO: implement. true=nn vs human. false=nn vs nn
  );
end entity comms_rx;

architecture comms_rx_arch of comms_rx is

  type state_t is (
    -- waiting for command
    IDLE_S,
    -- tilemap transfers
    TR_MAP_S,
    TR_MAP_SPAWNS_S,
    TR_MAP_NUM_SPAWN_S,
    TR_MAP_NUM_SPAWN_BITS_S,
    TR_MAP_WIDTH,
    TR_MAP_HEIGHT,
    -- ga config transfers
    TR_GA_MUTATION_RATES,
    TR_GA_MAX_GEN,
    TR_GA_RUN_UNTIL_STOP_CMD,
    TR_GA_TOURNAMENT_SIZE,
    TR_GA_POPULATION_SIZE_EXP,
    TR_GA_MODEL_HISTORY_SIZE,
    TR_GA_MODEL_HISTORY_INTERVAL,
    TR_GA_SEED,
    TR_GA_REFERENCE_COUNT,
    TR_GA_EVAL_INTERVAL,
    TR_GA_SEED_COUNT,
    TR_GA_FRAME_LIMIT,
    -- player input transfers
    TR_PLAYER_INPUT
  );

  signal state : state_t := IDLE_S;

  signal tr_counter : unsigned(15 downto 0) := to_unsigned(0, 16);

  subtype msg_t is std_logic_vector(7 downto 0);
  -- when idle, we wait for the following uart messages
  -- to transition to other states
  constant TILEMAP_MSG       : msg_t := x"01"; -- start tilemap transfer
  constant GA_CONFIG_MSG     : msg_t := x"02"; -- start ga_config transfer
  constant TRAINING_STOP_MSG : msg_t := x"03"; -- stop/pause the training early
  -- TODO: look into strategies for resetting/flushing state machine
  constant PLAYER_INPUT_MSG      : msg_t := x"04"; -- human player input transfer
  constant TEST_MSG              : msg_t := x"05"; -- test print to serial
  constant INFERENCE_GO_MSG      : msg_t := x"06"; -- init game, configure to use player input
  constant INFERENCE_STOP_MSG    : msg_t := x"07"; -- stop game
  constant TRAINING_RESUME_MSG   : msg_t := x"08"; -- resume training, only when stopped/paused early
  constant PLAY_AGAINST_NN_TRUE  : msg_t := x"09"; -- turn on play against nn
  constant PLAY_AGAINST_NN_FALSE : msg_t := x"0A"; -- turn off play against nn
  constant TRAINING_GO_MSG : msg_t := x"0B";

begin

  state_proc : process (clk) is
  begin
    if rising_edge(clk) then
      -- these are related to system control.
      -- they should be default until addressed via a message.
      training_go       <= false;
      training_pause    <= false;
      training_resume   <= false;
      inference_go      <= false;
      inference_stop    <= false;
      human_input       <= default_playerinput_t;
      human_input_valid <= false;
      test_go           <= false;

      -- we only do anything when we recieve a valid message
      if uart_rx_valid = '1' then
        case state is
          -- message transitions
          when IDLE_S =>
            -- always safe to reset counter here
            tr_counter <= to_unsigned(0, 16);
            -- we are idling. handle messages
            case uart_rx is
              when TILEMAP_MSG =>
                state <= TR_MAP_S;
              when GA_CONFIG_MSG =>
                state <= TR_GA_MUTATION_RATES;
              when TRAINING_STOP_MSG =>
                -- no state transition required here. we are just
                -- passing it along through training_stop.
                training_pause <= true;
              when PLAYER_INPUT_MSG =>
                state <= TR_PLAYER_INPUT;
              when TEST_MSG =>
                test_go <= true;
              when INFERENCE_GO_MSG =>
                inference_go <= true;
              when INFERENCE_STOP_MSG =>
                inference_stop <= true;
              when TRAINING_RESUME_MSG =>
                training_resume <= true;
              when PLAY_AGAINST_NN_TRUE =>
                play_against_nn <= true;
              when PLAY_AGAINST_NN_FALSE =>
                play_against_nn <= false;
              when TRAINING_GO_MSG =>
                training_go <= true;
              when others =>
                null;
            end case;

          -- data transfer transitions
          -- tilemap transfer
          when TR_MAP_S =>
            -- populate map

            -- map is 2d, but counter is 1d.
            -- effectively doing modulus by grabbing different bit ranges:
            tilemap.m(
            -- TODO: might need to swap x and y here.
            to_integer(tr_counter(MAP_TILES_BITS - 1 downto 0)),
            to_integer(tr_counter(2 * MAP_TILES_BITS - 1 downto MAP_TILES_BITS))
            ) <= uart_rx(2 downto 0);

            if tr_counter = MAP_MAX_SIZE_TILES * MAP_MAX_SIZE_TILES - 1 then
              -- go next
              state      <= TR_MAP_SPAWNS_S;
              tr_counter <= to_unsigned(0, 16);
            else
              -- still receiving data
              tr_counter <= tr_counter + 1;
            end if;
          when TR_MAP_SPAWNS_S =>
            -- populate spawn

            -- spawns is 1d, but > 1 byte.
            -- on even counts, populate x.
            -- on odd counts, populate y.
            if tr_counter(0) = '0' then
              tilemap.spawn(to_integer(tr_counter(15 downto 1))).x <= unsigned(uart_rx(MAP_TILES_BITS - 1 downto 0));
            else
              tilemap.spawn(to_integer(tr_counter(15 downto 1))).y <= unsigned(uart_rx(MAP_TILES_BITS - 1 downto 0));
            end if;

            if tr_counter = (2 * MAP_MAX_SPAWNS) - 1 then
              -- go next
              state      <= TR_MAP_NUM_SPAWN_S;
              tr_counter <= to_unsigned(0, 16);
            else
              -- still receiving data
              tr_counter <= tr_counter + 1;
            end if;
          when TR_MAP_NUM_SPAWN_S =>
            -- num_spawn is just a byte
            tilemap.num_spawn <= unsigned(uart_rx);
            -- go next
            state <= TR_MAP_NUM_SPAWN_BITS_S;
          when TR_MAP_NUM_SPAWN_BITS_S =>
            -- num_spawn_bits is just a byte
            tilemap.num_spawn_bits <= unsigned(uart_rx(3 downto 0));
            -- go next
            state <= TR_MAP_WIDTH;
          when TR_MAP_WIDTH =>
            -- map width is just a byte
            tilemap.width <= unsigned(uart_rx(MAP_TILES_BITS downto 0));
            -- go next
            state <= TR_MAP_HEIGHT;
          when TR_MAP_HEIGHT =>
            -- map height is just a byte
            tilemap.height <= unsigned(uart_rx(MAP_TILES_BITS downto 0));
            -- go back to idle
            state <= IDLE_S;

          -- ga config transfer
          when TR_GA_MUTATION_RATES =>
            -- mutation rates is a 1d array of bytes
            ga_config.mutation_rates(to_integer(tr_counter)) <= unsigned(uart_rx);

            if tr_counter = MAX_POPULATION_SIZE - 1 then
              -- go next
              state      <= TR_GA_MAX_GEN;
              tr_counter <= to_unsigned(0, 16);
            else
              -- still receiving data
              tr_counter <= tr_counter + 1;
            end if;
          when TR_GA_MAX_GEN =>
            -- max_get is 2 bytes
            if tr_counter(0) = '0' then
              ga_config.max_gen(15 downto 8) <= unsigned(uart_rx);
            else
              ga_config.max_gen(7 downto 0) <= unsigned(uart_rx);
            end if;

            if tr_counter = 1 then
              -- go next
              state      <= TR_GA_RUN_UNTIL_STOP_CMD;
              tr_counter <= to_unsigned(0, 16);
            else
              -- still receiving data
              tr_counter <= tr_counter + 1;
            end if;
          when TR_GA_RUN_UNTIL_STOP_CMD =>
            -- run_until_stop_cmd is a bool
            if uart_rx(0) = '0' then
              ga_config.run_until_stop_cmd <= false;
            else
              ga_config.run_until_stop_cmd <= true;
            end if;
            -- go next
            state <= TR_GA_TOURNAMENT_SIZE;
          when TR_GA_TOURNAMENT_SIZE =>
            -- tournament_size is a byte
            ga_config.tournament_size <= unsigned(uart_rx);
            -- go next
            state <= TR_GA_POPULATION_SIZE_EXP;
          when TR_GA_POPULATION_SIZE_EXP =>
            -- population_size is a byte
            ga_config.population_size_exp <= unsigned(uart_rx);
            -- go next
            state <= TR_GA_MODEL_HISTORY_SIZE;
          when TR_GA_MODEL_HISTORY_SIZE =>
            -- model_history_size is a byte
            ga_config.model_history_size <= unsigned(uart_rx);
            -- go next
            state <= TR_GA_MODEL_HISTORY_INTERVAL;
          when TR_GA_MODEL_HISTORY_INTERVAL =>
            -- model_history_interval is a byte
            ga_config.model_history_interval <= unsigned(uart_rx);
            -- go next
            state <= TR_GA_SEED;
          when TR_GA_SEED =>
            -- seed is 4 bytes
            if tr_counter = 0 then
              ga_config.seed(31 downto 24) <= uart_rx;
            elsif tr_counter = 1 then
              ga_config.seed(23 downto 16) <= uart_rx;
            elsif tr_counter = 2 then
              ga_config.seed(15 downto 8) <= uart_rx;
            else
              ga_config.seed(7 downto 0) <= uart_rx;
            end if;

            if tr_counter = 3 then
              -- go next
              state      <= TR_GA_REFERENCE_COUNT;
              tr_counter <= to_unsigned(0, 16);
            else
              -- still receiving data
              tr_counter <= tr_counter + 1;
            end if;
          when TR_GA_REFERENCE_COUNT =>
            -- reference_count is a byte
            ga_config.reference_count <= unsigned(uart_rx);
            -- go next
            state <= TR_GA_EVAL_INTERVAL;
          when TR_GA_EVAL_INTERVAL =>
            -- eval_interval is a byte
            ga_config.eval_interval <= unsigned(uart_rx);
            -- go next
            state <= TR_GA_SEED_COUNT;
          when TR_GA_SEED_COUNT =>
            -- seed_count is a byte
            ga_config.seed_count <= unsigned(uart_rx);
            -- go next
            state <= TR_GA_FRAME_LIMIT;
          when TR_GA_FRAME_LIMIT =>
            -- frame_limit is 2 bytes
            if tr_counter(0) = '0' then
              ga_config.frame_limit(15 downto 8) <= unsigned(uart_rx);
            else
              ga_config.frame_limit(7 downto 0) <= unsigned(uart_rx);
            end if;

            if tr_counter = 1 then
              -- go back to idle
              state      <= IDLE_S;
              tr_counter <= to_unsigned(0, 16);
            else
              tr_counter <= tr_counter + 1;
            end if;

          -- player input transfer
          when TR_PLAYER_INPUT =>
            -- three bools. just unpack from one byte.
            human_input.left  <= uart_rx(0) = '1';
            human_input.right <= uart_rx(1) = '1';
            human_input.jump  <= uart_rx(2) = '1';
            human_input_valid <= true;

            state <= IDLE_S;
          when others =>
            null;
        end case;
      end if;
    end if;
  end process;

end architecture comms_rx_arch;
