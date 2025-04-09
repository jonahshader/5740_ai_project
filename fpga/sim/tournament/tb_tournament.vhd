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
    tournament_size => to_unsigned(2, 8),
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

  signal input_population_fitness : fitness_array_t       := simple_fitness;
  signal winner_counts            : winner_counts_array_t := default_winner_counts_array_t;
  signal done                     : boolean               := false;

  signal clk_counter : unsigned(15 downto 0) := to_unsigned(0, 16);

begin

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

  counter_proc : process is
  begin
    if rising_edge(clk) then
      clk_counter <= clk_counter + 1;
    end if;
  end process;

  main : process is
  begin
    test_runner_setup(runner, RUNNER_CFG);
    go <= true;
    wait until rising_edge(clk);
    go <= false;

    -- Wait until tournament module asserts "done"
    wait until done;
    -- Allow a couple of clock cycles for observation
    wait for 2 * CLK_100MHZ_PERIOD;

    test_runner_cleanup(runner);
    wait;
  end process;

end architecture tb;
