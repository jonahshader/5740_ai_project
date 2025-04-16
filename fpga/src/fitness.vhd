library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.game_types.all;
use work.ga_types.all;

entity fitness is
  port (
    clk                       : in std_logic;
    go                        : in boolean;
    rng                       : in std_logic_vector(31 downto 0);
    game_score                : in signed(15 downto 0);
    playagame_done            : in boolean;
    init_playagame            : out boolean;
    nn1_index                 : out unsigned(7 downto 0);
    nn2_index                 : out unsigned(7 downto 0);
    done_fitness              : out boolean;
    score_valid               : out boolean;
    output_population_fitness : out fitness_array_t;
    ga_config                 : in ga_config_t
  );
end entity fitness;

architecture fitness_arch of fitness is

  constant NN1_START       : integer := 16;
  constant NN1_END         : integer := 143;
  constant NUM_NN1         : integer := NN1_END - NN1_START + 1;
  constant TOTAL_OPPONENTS : integer := 16; -- nn2 loops from 0 to 15

  type   state_t is (IDLE_S, INIT_S, WAIT_S);
  signal state : state_t := IDLE_S;

  signal nn1_ctr : integer := NN1_START;
  signal nn2_ctr : integer := 0;

  signal fitness_acc      : fitness_array_t := default_fitness_array_t;
  signal done_r           : boolean         := false;
  signal score_valid_r    : boolean         := false;
  signal init_playagame_r : boolean         := false;

begin

  done_fitness              <= done_r;
  score_valid               <= score_valid_r;
  output_population_fitness <= fitness_acc;
  init_playagame            <= init_playagame_r;
  nn1_index                 <= to_unsigned(nn1_ctr, 8);
  nn2_index                 <= to_unsigned(nn2_ctr, 8);

  process (clk) is
  begin
    if rising_edge(clk) then
      case state is
        when IDLE_S =>
          done_r           <= false;
          score_valid_r    <= false;
          init_playagame_r <= false;
          if go then
            nn1_ctr          <= NN1_START;
            nn2_ctr          <= 0;
            fitness_acc      <= default_fitness_array_t;
            init_playagame_r <= true;
            state            <= INIT_S;
          end if;
        when INIT_S =>
          init_playagame_r <= false;
          score_valid_r    <= false;
          state            <= WAIT_S;
        when WAIT_S =>
          if playagame_done then
            score_valid_r                    <= true;
            fitness_acc(nn1_ctr - NN1_START) <= fitness_acc(nn1_ctr - NN1_START) + game_score;

            if nn2_ctr = TOTAL_OPPONENTS - 1 then
              if nn1_ctr = NN1_END then
                done_r           <= true;
                init_playagame_r <= false;
                state            <= WAIT_S; -- allow done_r to propagate for 1 cycle
              else
                nn1_ctr          <= nn1_ctr + 1;
                nn2_ctr          <= 0;
                init_playagame_r <= true;
                state            <= INIT_S;
              end if;
            else
              nn2_ctr          <= nn2_ctr + 1;
              init_playagame_r <= true;
              state            <= INIT_S;
            end if;
          else
            score_valid_r <= false;
          end if;
        when others =>
          state <= IDLE_S;
      end case;
    end if;
  end process;

end architecture fitness_arch;
