library ieee;

library vunit_lib;
context vunit_lib.vunit_context;

use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.game_types.all;
use work.ga_types.all;

entity tb_tournament is
  generic (
    RUNNER_CFG : string
  );
end entity tb_tournament;

architecture tb of tb_tournament is

  constant CLK_100MHZ_PERIOD : time := 10 ns;

  signal clk : std_logic := '0';
  signal go  : boolean   := false;
  -- configure the config to use tournament_size = 2, pop_size = 128
  signal ga_config : ga_config_t := (
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

  signal input_population_fitness : fitness_array_t       := simple_fitness;
  signal winner_counts            : winner_counts_array_t := default_winner_counts_array_t;
  signal done                     : boolean               := false;

  signal clk_counter : unsigned(31 downto 0) := to_unsigned(0, 32);

  signal running : boolean := false;

  signal counting_up : boolean := true;

begin

  -- Timeout after 125 us.
  test_runner_watchdog(runner, 125 us);

  clk <= not clk after CLK_100MHZ_PERIOD / 2;

  tournament_entity : entity work.tournament
    port map (
      clk                      => clk,
      rng                      => std_logic_vector(clk_counter),
      go                       => go,
      ga_config                => ga_config,
      input_population_fitness => input_population_fitness,
      winner_counts            => winner_counts,
      done                     => done
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
    go      <= true;
    running <= true;
    wait until rising_edge(clk);
    go      <= false;

    while test_suite loop
      if run("default") then
        -- Wait until tournament module asserts "done"
        wait until done;
        wait for 2 * CLK_100MHZ_PERIOD;

        check_equal(
                    count_victors(winner_counts),
                    2 ** to_integer(ga_config.population_size_exp),
                    "Tournament incorrect victor count.");
      elsif run("backward") then
        counting_up <= false;

        -- Wait until tournament module asserts "done"
        wait until done;
        wait for 2 * CLK_100MHZ_PERIOD;

        check_equal(
                    count_victors(winner_counts),
                    2 ** to_integer(ga_config.population_size_exp),
                    "Tournament incorrect victor count.");
      elsif run("multiple") then
        -- Wait until tournament module asserts "done"
        wait until done;
        wait for 2 * CLK_100MHZ_PERIOD;

        check_equal(
                    count_victors(winner_counts),
                    2 ** to_integer(ga_config.population_size_exp),
                    "Tournament incorrect victor count.");

        go <= true;
        wait until rising_edge(clk);
        go <= false;

        -- Wait until tournament module asserts "done"
        wait until done;
        wait for 2 * CLK_100MHZ_PERIOD;

        check_equal(
                    count_victors(winner_counts),
                    2 ** to_integer(ga_config.population_size_exp),
                    "Tournament incorrect victor count.");
      end if;
    end loop;

    test_runner_cleanup(runner);
    wait;
  end process;

end architecture tb;
