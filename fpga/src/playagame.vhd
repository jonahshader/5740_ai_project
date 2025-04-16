library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.game_types.all;

entity playagame is
  port (
    clk          : in  std_logic;
    init         : in  boolean;
    seed_rng     : in  std_logic_vector(31 downto 0);
    nn1_index    : in  unsigned(7 downto 0);
    nn2_index    : in  unsigned(7 downto 0);
    score_output : out signed(15 downto 0);
    done         : out boolean;

    -- Internal game interface
    init_game  : out boolean;
    swap_start : out boolean;
    seed       : out std_logic_vector(31 downto 0);
    go         : out boolean;
    done_game  : in  boolean;
    gamestate  : in  gamestate_t
  );
end entity playagame;

architecture playagame_arch of playagame is

  type   state_t is (IDLE_S, INIT_GAME1, WAIT_GAME1, INIT_GAME2, WAIT_GAME2, DONE_S);
  signal state : state_t := IDLE_S;

  signal game_score_r : signed(15 downto 0) := (others => '0');
  signal done_r       : boolean             := false;
  signal init_game_r  : boolean             := false;
  signal go_r         : boolean             := false;
  signal swap_r       : boolean             := false;
  signal latched      : boolean             := false;

begin

  -- Output assignments
  score_output <= game_score_r;
  done         <= done_r;
  init_game    <= init_game_r;
  swap_start   <= swap_r;
  seed         <= seed_rng;
  go           <= go_r;

  process (clk) is
  begin
    if rising_edge(clk) then
      case state is
        when IDLE_S =>
          done_r       <= false;
          game_score_r <= (others => '0');
          go_r         <= false;
          if init and not latched then
            latched     <= true;
            init_game_r <= true;
            swap_r      <= false;
            state       <= INIT_GAME1;
          end if;
          if not init then
            latched <= false;
          end if;
        when INIT_GAME1 =>
          init_game_r <= false;
          if done_game then
            go_r  <= true;
            state <= WAIT_GAME1;
          end if;
        when WAIT_GAME1 =>
          if not done_game then
            go_r <= false;
          elsif done_game and not go_r then
            game_score_r <= gamestate.p1.score;
            init_game_r  <= true;
            swap_r       <= true;
            state        <= INIT_GAME2;
          end if;
        when INIT_GAME2 =>
          init_game_r <= false;
          if done_game then
            go_r  <= true;
            state <= WAIT_GAME2;
          end if;
        when WAIT_GAME2 =>
          if not done_game then
            go_r <= false;
          elsif done_game and not go_r then
            game_score_r <= game_score_r + gamestate.p1.score;
            done_r       <= true;
            state        <= DONE_S;
          end if;
        when DONE_S =>
          if not init then
            state <= IDLE_S;
          end if;
        when others =>
          state <= IDLE_S;

      end case;
    end if;
  end process;

end architecture playagame_arch;
