library ieee;

library vunit_lib;
context vunit_lib.vunit_context;

use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.game_types.all;
use work.ga_types.all;
use work.bram_types.all;

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
    population_size_exp => to_unsigned(2, 8), -- reduced to 2
    model_history_size => to_unsigned(2, 8),
    model_history_interval => to_unsigned(0, 8),
    seed => (others => '0'),
    reference_count => to_unsigned(2, 8),     -- one opponent
    eval_interval => to_unsigned(0, 8),
    seed_count => to_unsigned(2, 8),
    frame_limit => to_unsigned(0, 16)
  );
  signal fitness_go   : boolean     := false;
  signal fitness_done : boolean     := false;

  signal bm_command    : bram_command_t;
  signal bm_read_index : bram_index_t;
  signal bm_go         : boolean := false;
  signal bm_done       : boolean := false;

  signal rng            : std_logic_vector(31 downto 0) := x"00000001";
  signal seed           : std_logic_vector(31 downto 0);
  signal init_playagame : boolean                       := false;
  signal swap_start     : boolean                       := false;
  signal playagame_done : boolean                       := false;
  signal game_score     : signed(15 downto 0)           := to_signed(1, 16);

  signal output_population_fitness : fitness_array_t := default_fitness_array_t;

begin

  test_runner_watchdog(runner, 2 ms);

  clk <= not clk after CLK_100MHZ_PERIOD / 2;

  uut_fitness : entity work.fitness
    port map (
      clk                       => clk,
      rng                       => rng,
      bm_command                => bm_command,
      bm_read_index             => bm_read_index,
      bm_go                     => bm_go,
      bm_done                   => bm_done,
      ga_config                 => ga_config,
      fitness_go                => fitness_go,
      fitness_done              => fitness_done,
      seed                      => seed,
      init_playagame            => init_playagame,
      swap_start                => swap_start,
      playagame_done            => playagame_done,
      game_score                => game_score,
      output_population_fitness => output_population_fitness
    );

  main : process is
    variable bm_go_seen          : boolean := false;
    variable init_playagame_seen : boolean := false;
  begin
    test_runner_setup(runner, RUNNER_CFG);

    wait until rising_edge(clk);
    fitness_go <= true;
    wait until rising_edge(clk);
    fitness_go <= false;

    if run("default") then
      loop
        wait until rising_edge(clk);

        -- Pulse bm_done once per bm_go
        if bm_go and not bm_go_seen then
          bm_done    <= true;
          bm_go_seen := true;
        else
          bm_done <= false;
          if not bm_go then
            bm_go_seen := false;
          end if;
        end if;

        -- Pulse playagame_done once per init_playagame
        if init_playagame and not init_playagame_seen then
          playagame_done      <= true;
          init_playagame_seen := true;
        else
          playagame_done <= false;
          if not init_playagame then
            init_playagame_seen := false;
          end if;
        end if;

        exit when fitness_done;
      end loop;

      wait for CLK_100MHZ_PERIOD;

      check(
            output_population_fitness(0) /= to_signed(0, 16),
            "First fitness value should have been updated."
          );
    end if;

    test_runner_cleanup(runner);
    wait;
  end process;

end architecture tb;
