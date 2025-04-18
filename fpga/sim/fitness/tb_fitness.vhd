library ieee;

library vunit_lib;
context vunit_lib.vunit_context;

use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.game_types.all;
use work.ga_types.all;

entity tb_fitness is
  generic (
    RUNNER_CFG : string
  );
end entity tb_fitness;

architecture tb of tb_fitness is

  constant CLK_100MHZ_PERIOD : time := 10 ns;

  signal clk : std_logic := '0';

  signal ga_config    : ga_config_t := (
    mutation_rates => default_mutation_rates_t,
    max_gen => to_unsigned(0, 16),
    run_until_stop_cmd => false,
    tournament_size => to_unsigned(5, 8),
    population_size_exp => to_unsigned(7, 8),
    model_history_size => to_unsigned(0, 8),
    model_history_interval => to_unsigned(0, 8),
    seed => (others => '0'),
    reference_count => to_unsigned(0, 8),
    eval_interval => to_unsigned(0, 8),
    seed_count => to_unsigned(0, 8),
    frame_limit => to_unsigned(0, 16)
  );
  signal fitness_go   : boolean     := false;
  signal fitness_done : boolean     := false;

  signal nn1_index      : unsigned(7 downto 0);
  signal nn2_index      : unsigned(7 downto 0);
  signal seed           : std_logic_vector(31 downto 0);
  signal frame_limit    : boolean := false;
  signal init_playagame : boolean := false;

  signal playagame_done : boolean             := false;
  signal game_score     : signed(15 downto 0) := (others => '0');

  signal output_population_fitness : fitness_array_t := default_fitness_array_t;

  function simple_fitness return fitness_array_t is
    variable val : fitness_array_t := default_fitness_array_t;
  begin
    for i in 0 to MAX_POPULATION_SIZE - 1 loop
      val(i) := to_signed(i, 16);
    end loop;
    return val;
  end function;

  -- make sure the number of wins is equel to the population size
  function count_victors(winners : winner_counts_array_t) return integer is
    variable count : integer := 0;
  begin
    for i in 0 to MAX_POPULATION_SIZE - 1 loop
      count := count + to_integer(winners(i));
    end loop;

    return count;
  end function;

  signal clk_counter : unsigned(31 downto 0) := to_unsigned(0, 32);

  signal running : boolean := false;

  signal counting_up : boolean := true;

begin

  test_runner_watchdog(runner, 125 us);

  clk <= not clk after CLK_100MHZ_PERIOD / 2;

  uut_fitness : entity work.fitness
    port map (
      clk                       => clk,
      ga_config                 => ga_config,
      fitness_go                => fitness_go,
      fitness_done              => fitness_done,
      nn1_index                 => nn1_index,
      nn2_index                 => nn2_index,
      seed                      => seed,
      frame_limit               => frame_limit,
      init_playagame            => init_playagame,
      playagame_done            => playagame_done,
      game_score                => game_score,
      output_population_fitness => output_population_fitness
    );

  counter_proc : process (clk) is
  begin
    if rising_edge(clk) then
      if running then
        -- increment the counter
        if counting_up then
          clk_counter <= clk_counter + 1;
        else
          clk_counter <= clk_counter - 1;
        end if;
      end if;
    end if;
  end process;

  main : process is
  begin
    test_runner_setup(runner, RUNNER_CFG);

    wait until rising_edge(clk);
    fitness_go <= true;
    wait until rising_edge(clk);
    fitness_go <= false;

    if run("default") then
      wait until fitness_done;
      wait for CLK_PERIOD;
      check(
            all_nonzero(output_population_fitness, POPULATION_SIZE),
            "All fitness entries should be non-zero"
          );
    end if;

    test_runner_cleanup(runner);
    wait;
  end process;

end architecture tb;
