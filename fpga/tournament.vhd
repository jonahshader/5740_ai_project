library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.game_types.all;
use work.custom_utils.all;
use work.ga_types.all;

entity tournament is
  port (
    clk : in std_logic;

    -- rng parameters
    rng : in std_logic_vector(31 downto 0); -- rnadom number is 32 bits
    go  : in boolean;                       -- re-init the tournament using seed. goes high for one cycle.

    -- tournament population
    ga_config                : in ga_config_t;                                             -- configuration for the tournament
    input_population_fitness : in  fitness_array_t;                                        -- fitnsess of each chromosome
    winner_counts            : out winner_counts_array_t := default_winner_counts_array_t; -- indices of winners of the tournament

    done : out boolean
  );
end entity tournament;

architecture tournament_arch of tournament is

  signal population_size : unsigned(7 downto 0) := shift_left(to_unsigned(1, 8), to_integer(ga_config.population_size_exp));

  -- FSM state
  signal count : integer := 0;
  type   state_t is (IDLE_S, TOURNAMENT_ROUND_S);
  signal state : state_t := IDLE_S;

  -- tournament tracking
  signal tournament_round : integer              := 0;
  signal best_score       : signed(15 downto 0)  := (others => '0');
  signal best_index       : unsigned(6 downto 0) := (others => '0');

begin

  -- FSM: run one tournament round per clock cycle
  process (clk) is
    variable current_index     : unsigned(6 downto 0);
    variable fitness_candidate : signed(15 downto 0);
  begin
    if rising_edge(clk) then
      case state is
        when IDLE_S =>
          done <= FALSE;
          if go = TRUE then
            count            <= 0;
            tournament_round <= 0;
            best_score       <= (others => '0');
            best_index       <= (others => '0');
            for i in 0 to MAX_POPULATION_SIZE - 1 loop
              winner_counts(i) <= (others => '0');
            end loop;
            state <= TOURNAMENT_ROUND_S;
          end if;
        when TOURNAMENT_ROUND_S =>
          -- generate index safely
          current_index := unsigned(rng(6 downto 0));

          fitness_candidate := input_population_fitness(to_integer(current_index));

          if tournament_round = 0 or fitness_candidate > best_score then
            best_score <= fitness_candidate;
            best_index <= current_index;
          end if;

          tournament_round <= tournament_round + 1;

          if tournament_round = ga_config.tournament_size - 1 then
            -- end of tournament: update win count
            winner_counts(to_integer(best_index)) <= winner_counts(to_integer(best_index)) + 1;
            count                                 <= count + 1;
            tournament_round                      <= 0;
            best_score                            <= fitness_candidate;

            if count = population_size - 1 then
              done  <= TRUE;
              state <= IDLE_S;
            end if;
          end if;
        when others =>
          state <= IDLE_S;

      end case;
    end if;
  end process;

end architecture tournament_arch;
