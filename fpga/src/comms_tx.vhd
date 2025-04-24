library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.fixed_pkg.all;
use ieee.fixed_float_types.all;

use work.game_types.all;
use work.bram_types.all;
use work.ga_types.all;
use work.ne_types.all;
use work.nn_types.total_params;

entity comms_tx is
  port (
    -- master clock, uart io
    clk          : in std_logic;
    uart_tx      : out std_logic_vector(7 downto 0) := (others => '0');
    uart_tx_send : out std_logic                    := '0';
    uart_done    : in std_logic;

    -- transfers
    ga_state       : in ga_state_t;
    ga_state_send  : in boolean;
    gamestate      : in gamestate_t;
    gamestate_send : in boolean;
    test_go        : in boolean;

    -- debug
    db_bram_dump_param       : in param_t;
    db_bram_dump_param_index : in param_index_t;
    db_bram_dump_param_valid : in boolean;

    -- from neuroevolution
    ne_announce_new_state : in boolean;
    ne_state              : in ne_state_t;

    -- ready to initiate a transfer (i.e., not busy)
    ready : out boolean
  );
end entity comms_tx;

architecture comms_tx_arch of comms_tx is

  type state_t is (
    IDLE_S,
    -- ga status transfers
    TR_CURRENT_GEN_1_S,
    TR_CURRENT_GEN_2_S,
    TR_REFERENCE_FITNESS_1_S,
    TR_REFERENCE_FITNESS_2_S,
    -- gamestate transfers
    TR_P1_X_1_S,
    TR_P1_X_2_S,
    TR_P1_Y_1_S,
    TR_P1_Y_2_S,
    TR_P1_SCORE_1_S,
    TR_P1_SCORE_2_S,
    TR_P1_DEAD_TIMEOUT_S,
    TR_P2_X_1_S,
    TR_P2_X_2_S,
    TR_P2_Y_1_S,
    TR_P2_Y_2_S,
    TR_P2_SCORE_1_S,
    TR_P2_SCORE_2_S,
    TR_P2_DEAD_TIMEOUT_S,
    TR_COIN_X_S,
    TR_COIN_Y_S,
    TR_AGE_1_S,
    TR_AGE_2_S,
    -- debug bram transfer
    TR_BRAM_S
  );

  signal state : state_t := IDLE_S;
  -- goes high and stays high when uart_ready is pulsed
  signal uart_ready_r : boolean := true;

  -- debug
  signal db_bram_dump_param_valid_p    : boolean       := false;
  signal db_queue_bram_serial_transfer : boolean       := false;
  signal db_bram_param_index_send      : param_index_t := (others => '0');

  type   bram_reg_t is array (0 to BRAM_DEPTH - 1) of param_t;
  signal bram_reg : bram_reg_t := (others => (others => '0'));

  subtype  msg_t is std_logic_vector(7 downto 0);
  constant GA_STATUS_MSG  : msg_t := x"01";
  constant GAMESTATE_MSG  : msg_t := x"02";
  constant NE_IS_IDLE     : msg_t := x"03";
  constant NE_IS_TRAINING : msg_t := x"04";
  constant NE_IS_PLAYING  : msg_t := x"05";
  constant TEST_MSG       : msg_t := x"68"; -- 'h'
  constant SEND_BRAM_MSG  : msg_t := x"06";

begin

  -- this module is ready when we are idling and uart is ready
  ready <= state = IDLE_S and uart_ready_r;

  state_proc : process (clk) is
  begin
    if rising_edge(clk) then
      -- defaults
      uart_tx      <= (others => '0');
      uart_tx_send <= '0';

      -- db_bram_dump_param_valid edge detection.
      -- if we go from valid to not valid, that means
      -- the bram transfer into the internal registers finished.
      -- now we need to queue the transfer over uart.
      if db_bram_dump_param_valid_p and not db_bram_dump_param_valid then
        db_queue_bram_serial_transfer <= true;
      end if;
      db_bram_dump_param_valid_p <= db_bram_dump_param_valid;

      -- bram reg is always hooked up to input
      if db_bram_dump_param_valid then
        bram_reg(to_integer(db_bram_dump_param_index)) <= db_bram_dump_param;
      end if;

      -- grab uart_done pulse
      if uart_done = '1' then
        uart_ready_r <= true;
      end if;

      -- handle initiation of transfer
      if ready then
        if ga_state_send then
          -- send a message indicating we are about to transfer ga state
          uart_tx      <= GA_STATUS_MSG;
          uart_tx_send <= '1';
          uart_ready_r <= false;
          state        <= TR_CURRENT_GEN_1_S;
        elsif gamestate_send then
          -- send a message indicating we are about to transfer gamestate
          uart_tx      <= GAMESTATE_MSG;
          uart_tx_send <= '1';
          uart_ready_r <= false;
          state        <= TR_P1_X_1_S;
        elsif test_go then
          uart_tx      <= TEST_MSG;
          uart_tx_send <= '1';
          uart_ready_r <= false;
        -- no state transition
        elsif db_queue_bram_serial_transfer then
          -- about to send the bram
          uart_tx                       <= SEND_BRAM_MSG;
          uart_tx_send                  <= '1';
          uart_ready_r                  <= false;
          state                         <= TR_BRAM_S;
          db_bram_param_index_send      <= (others => '0');
          db_queue_bram_serial_transfer <= false;
        elsif ne_announce_new_state then
          -- send messages indicating the current ne state.
          case ne_state is
            when NE_IDLE_S =>
              uart_tx      <= NE_IS_IDLE;
              uart_tx_send <= '1';
              uart_ready_r <= false;
            when NE_TRAINING_S =>
              uart_tx      <= NE_IS_TRAINING;
              uart_tx_send <= '1';
              uart_ready_r <= false;
            when NE_PLAYING_S =>
              uart_tx      <= NE_IS_PLAYING;
              uart_tx_send <= '1';
              uart_ready_r <= false;
            when others =>
              null;
          end case;
        end if;
      end if;

      -- the rest of the state machine operates upon the uart_done signal pulse.
      -- also check uart_tx_send to prevent double-sends, because uart_tx might
      -- be holding uart_done high for 2 cycles instead of one...
      if uart_done = '1' and uart_tx_send = '0' then
        -- we are sending something in every state (except for IDLE_S, which overrides this)
        uart_tx_send <= '1';
        case state is
          when TR_CURRENT_GEN_1_S =>
            uart_tx <= std_logic_vector(ga_state.current_gen(15 downto 8));
            state   <= TR_CURRENT_GEN_2_S;
          when TR_CURRENT_GEN_2_S =>
            uart_tx <= std_logic_vector(ga_state.current_gen(7 downto 0));
            state   <= TR_REFERENCE_FITNESS_1_S;
          when TR_REFERENCE_FITNESS_1_S =>
            uart_tx <= std_logic_vector(ga_state.reference_fitness(15 downto 8));
            state   <= TR_REFERENCE_FITNESS_2_S;
          when TR_REFERENCE_FITNESS_2_S =>
            uart_tx <= std_logic_vector(ga_state.reference_fitness(7 downto 0));
            state   <= IDLE_S;
          when TR_P1_X_1_S =>
            -- convert to integer with truncation (no rounding) and wrapping (no saturation)
            -- then convert to std_logic_vector and take the upper byte
            -- uart_tx <= std_logic_vector(to_unsigned(to_integer(gamestate.p1.pos.x, fixed_wrap, fixed_truncate),
            --                                         16)(15 downto 8));
            uart_tx <= to_std_logic_vector(resize(gamestate.p1.pos.x, 15, 8, fixed_wrap, fixed_truncate));
            state   <= TR_P1_X_2_S;
          when TR_P1_X_2_S =>
            -- take the lower byte
            uart_tx <= to_std_logic_vector(resize(gamestate.p1.pos.x, 7, 0, fixed_wrap, fixed_truncate));
            state   <= TR_P1_Y_1_S;
          when TR_P1_Y_1_S =>
            uart_tx <= to_std_logic_vector(resize(gamestate.p1.pos.y, 15, 8, fixed_wrap, fixed_truncate));
            state   <= TR_P1_Y_2_S;
          when TR_P1_Y_2_S =>
            uart_tx <= to_std_logic_vector(resize(gamestate.p1.pos.y, 7, 0, fixed_wrap, fixed_truncate));
            state   <= TR_P1_SCORE_1_S;
          when TR_P1_SCORE_1_S =>
            uart_tx <= std_logic_vector(gamestate.p1.score(15 downto 8));
            state   <= TR_P1_SCORE_2_S;
          when TR_P1_SCORE_2_S =>
            uart_tx <= std_logic_vector(gamestate.p1.score(7 downto 0));
            state   <= TR_P1_DEAD_TIMEOUT_S;
          when TR_P1_DEAD_TIMEOUT_S =>
            uart_tx <= std_logic_vector(gamestate.p1.dead_timeout);
            state   <= TR_P2_X_1_S;
          when TR_P2_X_1_S =>
            uart_tx <= to_std_logic_vector(resize(gamestate.p2.pos.x, 15, 8, fixed_wrap, fixed_truncate));
            state   <= TR_P2_X_2_S;
          when TR_P2_X_2_S =>
            uart_tx <= to_std_logic_vector(resize(gamestate.p2.pos.x, 7, 0, fixed_wrap, fixed_truncate));
            state   <= TR_P2_Y_1_S;
          when TR_P2_Y_1_S =>
            uart_tx <= to_std_logic_vector(resize(gamestate.p2.pos.y, 15, 8, fixed_wrap, fixed_truncate));
            state   <= TR_P2_Y_2_S;
          when TR_P2_Y_2_S =>
            uart_tx <= to_std_logic_vector(resize(gamestate.p2.pos.y, 7, 0, fixed_wrap, fixed_truncate));
            state   <= TR_P2_SCORE_1_S;
          when TR_P2_SCORE_1_S =>
            uart_tx <= std_logic_vector(gamestate.p2.score(15 downto 8));
            state   <= TR_P2_SCORE_2_S;
          when TR_P2_SCORE_2_S =>
            uart_tx <= std_logic_vector(gamestate.p2.score(7 downto 0));
            state   <= TR_P2_DEAD_TIMEOUT_S;
          when TR_P2_DEAD_TIMEOUT_S =>
            uart_tx <= std_logic_vector(gamestate.p2.dead_timeout);
            state   <= TR_COIN_X_S;
          when TR_COIN_X_S =>
            uart_tx <= std_logic_vector(resize(gamestate.coin_pos.x, 8));
            state   <= TR_COIN_Y_S;
          when TR_COIN_Y_S =>
            uart_tx <= std_logic_vector(resize(gamestate.coin_pos.y, 8));
            state   <= TR_AGE_1_S;
          when TR_AGE_1_S =>
            uart_tx <= std_logic_vector(gamestate.age(15 downto 8));
            state   <= TR_AGE_2_S;
          when TR_AGE_2_S =>
            -- take the lower byte
            uart_tx <= std_logic_vector(gamestate.age(7 downto 0));
            state   <= IDLE_S;
          when TR_BRAM_S =>
            uart_tx <= "0000" & bram_reg(to_integer(db_bram_param_index_send));
            if db_bram_param_index_send = BRAM_DEPTH - 1 then
              -- go idle
              state <= IDLE_S;
            else
              db_bram_param_index_send <= db_bram_param_index_send + 1;
            end if;
          when IDLE_S =>
            uart_tx_send <= '0';    -- don't send anything
          when others =>
            null;
        end case;
      end if;
    end if;
  end process;

end architecture comms_tx_arch;
