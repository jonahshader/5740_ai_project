library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.game_types.all;
use work.ga_types.all;
use work.bram_types.all;

entity fitness is
  port (
    clk : in std_logic;

    -- interface with bram_manager (bm)
    bm_command    : out bram_command_t := C_READ_TO_NN_1;
    bm_read_index : out bram_index_t   := (others => '0');
    -- (no write_index)
    bm_go   : out boolean := false;
    bm_done : in boolean;

    -- interface with GA controller
    ga_config    : in ga_config_t; -- population_size_exp, model_history_size, reference_count, seed, frame_limit
    fitness_go   : in boolean;
    fitness_done : out boolean;

    -- interface with playagame
    seed           : out std_logic_vector(31 downto 0); -- seed from ga_config
    frame_limit    : out unsigned(15 downto 0);         -- frame_limit from ga_config
    init_playagame : out boolean;

    playagame_done : in boolean;             -- goes high when playagame is done playing two players from both start locations
    game_score     : in signed(15 downto 0); -- score from playagame, (p1 vs p2 score + p2 vs p1 score)

    -- results
    output_population_fitness : out fitness_array_t -- an array of size 2^population_size_exp, each index has the fitness of the respective choromosome
  );
end entity fitness;

architecture fitness_arch of fitness is

  -- looping limits
  signal population_size : unsigned(7 downto 0) := shift_left(to_unsigned(1, 8), to_integer(ga_config.population_size_exp)); -- 2^population_size_exp
  signal total_opponents : unsigned(7 downto 0) := ga_config.model_history_size + ga_config.reference_count;
  signal nn1_start       : unsigned(7 downto 0) := ga_config.model_history_size + ga_config.reference_count - 1;
  signal nn1_end         : unsigned(7 downto 0) := (ga_config.model_history_size + ga_config.reference_count - 1) + shift_left(to_unsigned(1, 8), to_integer(ga_config.population_size_exp));

  -- loop counters
  signal nn1_ctr : unsigned(7 downto 0) := ga_config.model_history_size + ga_config.reference_count - 1;
  signal nn2_ctr : unsigned(7 downto 0) := (others => '0');

  signal done_r           : boolean := false; -- done with everything
  signal init_playagame_r : boolean := false; -- pulse to playagame to start a game

  type   state_t is (IDLE_S, INIT_S, WAIT_S);
  signal state : state_t := IDLE_S;

begin

  -- Output assignments
  seed        <= ga_config.seed;
  frame_limit <= ga_config.frame_limit;
  nn1_index   <= nn1_ctr;
  nn2_index   <= nn2_ctr;

  init_playagame <= init_playagame_r;
  fitness_done   <= done_r;

  process (clk) is
  begin
    if rising_edge(clk) then
      case state is
        when IDLE_S =>
          done_r           <= false;
          init_playagame_r <= false;
          if fitness_go then
            nn2_ctr          <= (others => '0');
            nn1_ctr          <= ga_config.model_history_size + ga_config.reference_count - 1;
            done_r           <= false;
            init_playagame_r <= true;
            for i in 0 to MAX_POPULATION_SIZE - 1 loop
              output_population_fitness(i) <= (others => '0');
            end loop;
            state <= INIT_S;
          end if;
        when INIT_S =>
          init_playagame_r <= false; -- pulse done
          state            <= WAIT_S;
        when WAIT_S =>
          if playagame_done then
            -- Store score
            output_population_fitness(to_integer(nn1_ctr) - to_integer(nn1_start)) <= output_population_fitness(to_integer(nn1_ctr) - to_integer(nn1_start)) + game_score;

            if nn2_ctr = total_opponents - 1 then
              if nn1_ctr = nn1_end then
                done_r <= true;
                state  <= IDLE_S; -- wait for new fitness_go
              else
                nn1_ctr          <= nn1_ctr + 1;
                nn2_ctr          <= (others => '0');
                init_playagame_r <= true;
                state            <= INIT_S;
              end if;
            else
              nn2_ctr          <= nn2_ctr + 1;
              init_playagame_r <= true;
              state            <= INIT_S;
            end if;
          end if;
        when others =>
          null;

      end case;
    end if;
  end process;

end architecture fitness_arch;
