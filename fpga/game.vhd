library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.game_types.all;
use work.player_funs.all;
use work.custom_utils.all;

entity game is
  port (
    clk        : in std_logic;
    init       : in boolean;                       -- re-init the game using seed. goes high for one cycle
    swap_start : in boolean;                       -- swaps initial player spawns
    seed       : in std_logic_vector(31 downto 0); -- seed is 32 bits
    m          : in tilemap_t;

    -- player inputs can come from players or ai
    p1_input : in playerinput_t;
    p2_input : in playerinput_t;

    go        : in boolean;  -- start an update. only call when done is high
    done      : out boolean; -- also goes high after init is done (high when in idle_s)
    gamestate : out gamestate_t
  );
end entity game;

architecture game_arch of game is

  -- rng stuff
  constant NUM_RNG         : integer := 3;                                -- three random numbers: p1 spawn, p2 spawn, coin spawn
  constant INIT_CYCLES     : integer := 6;
  signal   enable_rng      : boolean := false;
  signal   result_rng      : std_logic_vector(32 * NUM_RNG - 1 downto 0); -- rng output is a multiple of 32 bits
  signal   p1_spawn_tile   : tilepos_t;
  signal   p2_spawn_tile   : tilepos_t;
  signal   coin_spawn_tile : tilepos_t;

  signal init_counter : unsigned(2 downto 0) := to_unsigned(INIT_CYCLES, 3);

  type   state_t is (IDLE_S, INIT_S, PHASE1_S, PHASE2_S, RESPAWN_S);
  signal state : state_t := IDLE_S;

  -- gs contains all the game state
  signal gs : gamestate_t := default_gamestate_t;

begin

  -- connecting wires
  p1_spawn_tile   <= sample_spawn(m, result_rng(31 downto 0));
  p2_spawn_tile   <= sample_spawn(m, result_rng(63 downto 32));
  coin_spawn_tile <= sample_spawn(m, result_rng(95 downto 64));
  enable_rng      <= state = INIT_S or state = PHASE1_S;
  done            <= state = IDLE_S;

  -- instantiate
  rng : entity work.xormix32
    generic map (
      STREAMS => NUM_RNG
    )
    port map (
      -- clock and reset
      clk => clk,
      rst => to_std_logic(init),

      -- config
      seed_x => seed,
      seed_y => (others => '0'),

      -- rng
      enable => to_std_logic(enable_rng),
      result => result_rng
    );

  -- state machine
  state_proc : process (clk) is
  begin
    if rising_edge(clk) then
      -- always go straight to init state when init is true
      if init then
        state        <= INIT_S;
        init_counter <= to_unsigned(INIT_CYCLES, 3);

      -- TODO: initialize stuff
      else
        --
        case state is
          when IDLE_S =>
            if go then
              state <= PHASE1_S;
            end if;
          when INIT_S =>
            if init_counter = 0 then
              -- done initializing, go back to idle
              state <= IDLE_S;
              -- set starting positions
              gs        <= default_gamestate_t;
              gs.p1.pos <= to_f4_vec(p1_spawn_tile);
            end if;
            init_counter <= init_counter - 1;
            if to_std_logic(go) = '1' then
              state <= PHASE1_S;
            end if;
          when PHASE1_S =>
            -- player1 <= phase_1(m, p1_input, p2_input, p1_spawn_tile, p2_spawn_tile, coin_spawn_tile, gamestate);
            state <= PHASE2_S;
          when PHASE2_S =>
            state <= RESPAWN_S;
          when RESPAWN_S =>
          -- gamestate <= state <= IDLE_S;
          when others =>
            null;
        end case;
      end if;
    end if;
  end process;

end architecture game_arch;
