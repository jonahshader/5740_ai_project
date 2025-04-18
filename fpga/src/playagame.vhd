library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.game_types.all;

entity playagame is
  port (
    clk : in  std_logic;

    -- interface with bramManager
    nn1_index_out : out unsigned(7 downto 0); -- index of bram to be loaded in nn1
    nn2_index_out : out unsigned(7 downto 0); -- index of bram to be loaded in nn2
    load_nns      : out boolean;              -- load nns from brams
    nns_loaded    : in  boolean;              -- nns loaded successfully

    -- interface with fitness
    swap_start_from_fitness : in  boolean;                       -- swap start from fitness
    seed_from_fitness       : in  std_logic_vector(31 downto 0); -- seed from fitness.ga_config
    frame_limit             : in  unsigned(15 downto 0);         -- frame_limit from fitness.ga_config
    nn1_index_in            : in  unsigned(7 downto 0);          -- index from fitness for nn1
    nn2_index_in            : in  unsigned(7 downto 0);          -- index from fitness for nn2
    game_go                 : in  boolean;                       -- start playing a game
    game_done               : out boolean;                       -- goes to fitness
    score_output            : out signed(15 downto 0);           -- final score from player 1 after a single game

    -- interface with game
    swap_start   : out boolean;                       -- swaps initial player spawns locations
    seed_to_game : out std_logic_vector(31 downto 0); -- seed to game
    game_init    : out boolean;                       -- goes high for one cycle when game is initialized
    frame_go     : out boolean;                       -- start an update. only call when done is high
    frame_done   : in  boolean;                       -- goes high when game.vhd is done with a frame.
    gamestate    : in  gamestate_t                    -- to read the score
  );
end entity playagame;

architecture playagame_arch of playagame is

  type   state_t is (
    IDLE_S, LOAD_NNS_S,
    INIT_GAME1_S, WAIT_GAME1_S,
    DONE_S
  );
  signal state : state_t := IDLE_S;

  signal score         : signed(15 downto 0)   := (others => '0');
  signal frame_counter : unsigned(15 downto 0) := (others => '0');

  signal game_init_r : boolean := false;
  signal frame_go_r  : boolean := false;
  signal load_nns_r  : boolean := false;
  signal game_done_r : boolean := false;

begin

  -- Outputs
  nn1_index_out <= nn1_index_in;
  nn2_index_out <= nn2_index_in;
  load_nns      <= load_nns_r;

  game_done    <= game_done_r;
  score_output <= score;

  swap_start   <= swap_start_from_fitness;
  seed_to_game <= seed_from_fitness;
  game_init    <= game_init_r;
  frame_go     <= frame_go_r;

  process (clk) is
  begin
    if rising_edge(clk) then
      game_init_r <= false;
      frame_go_r  <= false;
      load_nns_r  <= false;
      game_done_r <= false;

      case state is
        when IDLE_S =>
          score         <= (others => '0');
          frame_counter <= (others => '0');
          if game_go then
            load_nns_r <= true;
            state      <= LOAD_NNS_S;
          end if;
        when LOAD_NNS_S =>
          if nns_loaded then
            game_init_r <= true;
            state       <= INIT_GAME1_S;
          end if;
        when INIT_GAME1_S =>
          frame_go_r <= true;
          state      <= WAIT_GAME1_S;
        when WAIT_GAME1_S =>
          if frame_done then
            if frame_limit = 0 or frame_counter < frame_limit - 1 then
              frame_counter <= frame_counter + 1;
              frame_go_r    <= true;
            else
              score       <= gamestate.p1.score;
              game_done_r <= true;
              state       <= DONE_S;
            end if;
          end if;
        when DONE_S =>
          if not game_go then
            state <= IDLE_S;
          end if;
        when others =>
          state <= IDLE_S;
      end case;
    end if;
  end process;

end architecture playagame_arch;
