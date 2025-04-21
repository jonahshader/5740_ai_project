library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.game_types.all;
use work.ga_types.all;
use work.bram_types.all;

entity fitness is
  port (
    clk : in std_logic;
    rng : in std_logic_vector(31 downto 0);

    -- interface with bram_manager (bm)
    bm_command    : out bram_command_t := C_READ_TO_NN_1; -- can be C_READ_TO_NN_1 or C_READ_TO_NN_2
    bm_read_index : out bram_index_t   := (others => '0');
    -- (no write_index)
    bm_go   : out boolean := false;
    bm_done : in boolean;

    -- interface with GA controller
    ga_config    : in ga_config_t; -- population_size_exp, model_history_size, reference_count, seed
    fitness_go   : in boolean;
    fitness_done : out boolean;

    -- interface with playagame
    seed           : out std_logic_vector(31 downto 0); -- seed from seed_array
    init_playagame : out boolean;
    swap_start     : out boolean;

    playagame_done : in boolean;             -- goes high when playagame is done playing two players from both start locations
    game_score     : in signed(15 downto 0); -- score from playagame, (p1 vs p2 score + p2 vs p1 score)

    -- results
    output_population_fitness : out fitness_array_t := default_fitness_array_t -- an array of size 2^population_size_exp, each index has the fitness of the respective choromosome
  );
end entity fitness;

architecture fitness_arch of fitness is

  type   seeds_array_t is array(0 to MAX_SEED_COUNT - 1) of std_logic_vector(31 downto 0);
  signal seeds_array : seeds_array_t := (others => (others => '0'));

  signal current_chromosome : unsigned(7 downto 0);
  signal current_opponent   : unsigned(7 downto 0);
  signal seed_ctr           : integer range 0 to MAX_SEED_COUNT - 1 := 0;
  signal swap_state         : boolean                               := FALSE;

  signal current_nn1_index : bram_index_t := (others => '0');
  signal current_nn2_index : bram_index_t := (others => '0');

  signal fitness_accumulator : signed(15 downto 0) := (others => '0');

  type   state_t is (
    IDLE_S, INIT_SEEDS_S,
    CHECK_NN1_S, WAIT_NN1_S,
    CHECK_NN2_S, WAIT_NN2_S,
    START_GAME_S, WAIT_GAME_S,
    ACCUMULATE_S, ADVANCE_S,
    DONE_S
  );
  signal state         : state_t                           := IDLE_S;
  signal seed_rng      : std_logic_vector(31 downto 0);
  signal seed_init_ctr : integer range 0 to MAX_SEED_COUNT := 0;

begin

  process (clk) is

    variable population_size : unsigned(7 downto 0);
    variable total_opponents : unsigned(7 downto 0);
    variable opponent_start  : unsigned(7 downto 0);
    variable nn1_end         : unsigned(7 downto 0);

  begin
    if rising_edge(clk) then
      population_size := shift_left(to_unsigned(1, 8), to_integer(ga_config.population_size_exp));
      total_opponents := ga_config.model_history_size + ga_config.reference_count;
      opponent_start  := population_size;
      nn1_end         := population_size - 1;

      init_playagame <= false;
      bm_go          <= false;
      fitness_done   <= false;

      case state is
        when IDLE_S =>
          if fitness_go then
            current_chromosome  <= (others => '0');
            current_opponent    <= population_size;
            seed_ctr            <= 0;
            swap_state          <= FALSE;
            fitness_accumulator <= (others => '0');
            seed_rng            <= rng;
            seed_init_ctr       <= 0;
            state               <= INIT_SEEDS_S;
          end if;
        when INIT_SEEDS_S =>
          seeds_array(seed_init_ctr) <= seed_rng;
          seed_rng                   <= std_logic_vector(unsigned(seed_rng) + 1);
          if seed_init_ctr = ga_config.seed_count - 1 then
            state <= CHECK_NN1_S;
          else
            seed_init_ctr <= seed_init_ctr + 1;
          end if;
        when CHECK_NN1_S =>
          if current_chromosome /= current_nn1_index then
            bm_command    <= C_READ_TO_NN_1;
            bm_read_index <= current_chromosome;
            bm_go         <= true;
            state         <= WAIT_NN1_S;
          else
            state <= CHECK_NN2_S;
          end if;
        when WAIT_NN1_S =>
          if bm_done then
            current_nn1_index <= current_chromosome;
            state             <= CHECK_NN2_S;
          end if;
        when CHECK_NN2_S =>
          if current_opponent /= current_nn2_index then
            bm_command    <= C_READ_TO_NN_2;
            bm_read_index <= current_opponent;
            bm_go         <= true;
            state         <= WAIT_NN2_S;
          else
            state <= START_GAME_S;
          end if;
        when WAIT_NN2_S =>
          if bm_done then
            current_nn2_index <= current_opponent;
            state             <= START_GAME_S;
          end if;
        when START_GAME_S =>
          init_playagame <= true;
          seed           <= seeds_array(seed_ctr);
          swap_start     <= swap_state;
          state          <= WAIT_GAME_S;
        when WAIT_GAME_S =>
          if playagame_done then
            state <= ACCUMULATE_S;
          end if;
        when ACCUMULATE_S =>
          fitness_accumulator <= fitness_accumulator + game_score;
          state               <= ADVANCE_S;
        when ADVANCE_S =>
          if swap_state = FALSE then
            swap_state <= TRUE;
            state      <= START_GAME_S;      -- re-trigger game
          elsif seed_ctr < ga_config.seed_count - 1 then
            seed_ctr   <= seed_ctr + 1;
            swap_state <= FALSE;
            state      <= START_GAME_S;      -- re-trigger game
          elsif current_opponent < opponent_start + total_opponents - 1 then
            current_opponent <= current_opponent + 1;
            seed_ctr         <= 0;
            swap_state       <= FALSE;
            state            <= CHECK_NN2_S; -- may need to reload NN2
          elsif current_chromosome < nn1_end then
            output_population_fitness(to_integer(current_chromosome)) <= fitness_accumulator;
            current_chromosome                                        <= current_chromosome + 1;
            current_opponent                                          <= opponent_start;
            seed_ctr                                                  <= 0;
            swap_state                                                <= FALSE;
            fitness_accumulator                                       <= (others => '0');
            state                                                     <= CHECK_NN1_S;
          else
            output_population_fitness(to_integer(current_chromosome)) <= fitness_accumulator;
            fitness_done                                              <= true;
            state                                                     <= DONE_S;
          end if;
        when DONE_S =>
          if not fitness_go then
            state <= IDLE_S;
          end if;
        when others =>
          state <= IDLE_S;
      end case;
    end if;
  end process;

end architecture fitness_arch;

