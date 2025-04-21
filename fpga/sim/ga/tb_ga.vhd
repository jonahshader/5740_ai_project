library ieee;

library vunit_lib;
context vunit_lib.vunit_context;

use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.bram_types.all;
use work.ga_types.all;
use work.custom_utils.all;

entity tb_ga is
  generic (
    RUNNER_CFG : string
  );
end entity tb_ga;

architecture tb_arch of tb_ga is

  constant CLK_100MHZ_PERIOD : time      := 10 ns;
  signal   clk               : std_logic := '0';

  -- GA inputs
  signal config : ga_config_t := default_ga_config_t;
  signal go     : boolean     := false;
  signal pause  : boolean     := false;
  signal resume : boolean     := false;

  -- GA outputs
  signal done : boolean := false;
  signal rng  : std_logic_vector(31 downto 0);

  -- BRAM manager interface
  signal bm_command       : bram_command_t;
  signal bm_read_index    : bram_index_t;
  signal bm_write_index   : bram_index_t;
  signal bm_mutation_rate : mutation_rate_t;
  signal bm_go            : boolean;
  signal bm_done          : boolean := true;

  -- Tournament interface
  signal tn_go            : boolean;
  signal tn_done          : boolean               := true;
  signal tn_winner_counts : winner_counts_array_t := default_winner_counts_array_t;

  -- Fitness interface
  signal fn_go   : boolean;
  signal fn_done : boolean := true;

  -- Signals to track operations - only modified in counter_control_proc
  signal bram_init_count  : integer := 0;
  signal fitness_count    : integer := 0;
  signal tournament_count : integer := 0;
  signal prior_best_count : integer := 0;

  -- Signal to reset counters
  signal reset_counters : boolean := false;

  -- Tracking signals
  signal in_init_phase : boolean := false;
  signal prev_bm_go    : boolean := false;
  signal prev_fn_go    : boolean := false;
  signal prev_tn_go    : boolean := false;

  -- Signal to identify the BRAM init phase specifically
  signal init_phase_started : boolean := false;

begin

  -- Instantiate the device under test
  dut : entity work.ga
    port map (
      clk    => clk,
      config => config,
      go     => go,
      done   => done,
      pause  => pause,
      resume => resume,
      rng    => rng,

      bm_command       => bm_command,
      bm_read_index    => bm_read_index,
      bm_write_index   => bm_write_index,
      bm_mutation_rate => bm_mutation_rate,
      bm_go            => bm_go,
      bm_done          => bm_done,

      tn_go            => tn_go,
      tn_done          => tn_done,
      tn_winner_counts => tn_winner_counts,

      fn_go   => fn_go,
      fn_done => fn_done
    );

  -- Clock generation
  test_runner_watchdog(runner, 1 ms);
  clk <= not clk after CLK_100MHZ_PERIOD / 2;

  -- Counter and event tracking process
  counter_proc : process (clk) is
  begin
    if rising_edge(clk) then
      -- Track go signals from previous cycle for edge detection
      prev_bm_go <= bm_go;
      prev_fn_go <= fn_go;
      prev_tn_go <= tn_go;

      -- Reset counters if requested
      if reset_counters then
        bram_init_count    <= 0;
        fitness_count      <= 0;
        tournament_count   <= 0;
        prior_best_count   <= 0;
        in_init_phase      <= false;
        init_phase_started <= false;
      else
        -- Phase tracking
        if go then
          in_init_phase      <= true;
          init_phase_started <= false;
        elsif fn_go and not prev_fn_go then
          in_init_phase <= false;
        end if;

        -- Detection of initialization phase start
        -- The GA starts initializing BRAMs shortly after go
        if in_init_phase and bm_go and not prev_bm_go and not init_phase_started then
          init_phase_started <= true;
        end if;

        -- Increment counters on rising edges of respective signals
        if bm_go and not prev_bm_go then
          -- BRAM initializations happen first and have equal read/write indices
          if in_init_phase and bm_read_index = bm_write_index and init_phase_started then
            bram_init_count <= bram_init_count + 1;
          -- Prior best copies happen when fitness phase is done, and have specific signature
          elsif not in_init_phase and
                bm_read_index = to_unsigned(0, bm_read_index'length) and
                bm_mutation_rate = to_unsigned(0, bm_mutation_rate'length) and
            -- Only count prior best copies during specific phase, after tournament completes
                fitness_count > 0 and tournament_count > 0 then
            prior_best_count <= prior_best_count + 1;
          end if;
        end if;

        if fn_go and not prev_fn_go then
          fitness_count <= fitness_count + 1;
        end if;

        if tn_go and not prev_tn_go then
          tournament_count <= tournament_count + 1;
        end if;
      end if;
    end if;
  end process;

  -- Simple RNG simulator process
  rng_simulator : process (clk) is

    variable counter : unsigned(31 downto 0) := (others => '0');

  begin
    if rising_edge(clk) then
      if to_std_logic(go) = '1' then
        -- Use seed from config as initial value
        counter := unsigned(config.seed);
      else
        -- Simple counter-based pseudo-random sequence
        counter := counter + 1;
      end if;

      -- Output the counter value
      rng <= std_logic_vector(counter);
    end if;
  end process;

  -- BRAM manager response simulator
  bram_manager_proc : process is
  begin
    wait until bm_go;

    -- Simulate processing time
    bm_done <= false;
    wait for 3 * CLK_100MHZ_PERIOD;
    bm_done <= true;

    wait until not bm_go;
  end process;

  -- Fitness module simulator
  fitness_proc : process is
  begin
    wait until fn_go;

    -- Simulate processing time
    fn_done <= false;
    wait for 5 * CLK_100MHZ_PERIOD;
    fn_done <= true;

    wait until not fn_go;
  end process;

  -- Tournament simulator
  tournament_proc : process is
  begin
    wait until tn_go;

    -- Simulate processing time
    tn_done <= false;
    wait for 5 * CLK_100MHZ_PERIOD;

    -- Generate a tournament result with some winners
    for i in 0 to MAX_POPULATION_SIZE - 1 loop
      if i mod 4 = 0 then
        tn_winner_counts(i) <= to_unsigned(2, 8); -- Multi-victor
      elsif i mod 4 = 1 then
        tn_winner_counts(i) <= to_unsigned(1, 8); -- Single victor
      else
        tn_winner_counts(i) <= to_unsigned(0, 8); -- Non-victor
      end if;
    end loop;

    tn_done <= true;
    wait until not tn_go;
  end process;

  -- Main test process
  test_process : process is

    variable pop_size           : integer;
    variable model_history_size : integer;
    variable reference_count    : integer;
    variable total_brams        : integer;
    variable fitness_at_pause   : integer;

  begin
    test_runner_setup(runner, RUNNER_CFG);

    while test_suite loop
      -- Reset all counters before each test
      wait until rising_edge(clk);
      reset_counters <= true;
      wait until rising_edge(clk);
      reset_counters <= false;

      if run("test_bram_initialization") then
        -- Test that the GA properly initializes BRAMs

        -- Set configuration parameters
        config.population_size_exp <= to_unsigned(4, config.population_size_exp'length); -- Population of 16
        config.model_history_size  <= to_unsigned(4, config.model_history_size'length);
        config.reference_count     <= to_unsigned(2, config.reference_count'length);
        config.seed                <= (others => '0');                                   -- Set seed for reproducibility

        -- Calculate expected number of BRAMs to initialize
        pop_size           := 2 ** 4; -- 16
        model_history_size := 4;
        reference_count    := 2;
        total_brams        := pop_size + model_history_size + reference_count;

        -- Start the GA
        wait until rising_edge(clk);
        go <= true;
        wait until rising_edge(clk);
        go <= false;

        -- Wait for initialization to complete (fitness module gets activated)
        for i in 1 to 1000 loop
          if fn_go then
            exit;
          end if;
          wait for CLK_100MHZ_PERIOD;
        end loop;

        -- Wait a bit more to ensure all counts are registered
        wait for 10 * CLK_100MHZ_PERIOD;

        -- Check correct number of BRAMs were initialized
        check_equal(bram_init_count, total_brams,
                    "Expected " & integer'image(total_brams) &
                    " BRAM initializations, got " & integer'image(bram_init_count));
      elsif run("test_phase_sequencing") then
        -- Test the GA cycles through phases correctly

        -- Set configuration parameters
        config.population_size_exp    <= to_unsigned(3, config.population_size_exp'length);    -- Population of 8
        config.model_history_size     <= to_unsigned(2, config.model_history_size'length);
        config.reference_count        <= to_unsigned(1, config.reference_count'length);
        config.model_history_interval <= to_unsigned(1, config.model_history_interval'length); -- Save every generation
        config.max_gen                <= to_unsigned(3, config.max_gen'length);                -- Run for 3 generations
        config.run_until_stop_cmd     <= false;                                                -- Stop after max_gen
        config.seed                   <= (others => '0');                                      -- Set seed for reproducibility

        -- Start the GA
        wait until rising_edge(clk);
        go <= true;
        wait until rising_edge(clk);
        go <= false;

        -- Wait for completion - increase timeout for this test
        for i in 1 to 2000 loop
          if done then
            exit;
          end if;
          wait for CLK_100MHZ_PERIOD;
        end loop;

        check(done, "GA should have completed after max_gen");

        -- Wait a bit more to ensure all counts are registered
        wait for 10 * CLK_100MHZ_PERIOD;

        -- Check that each phase happened the expected number of times
        check_equal(fitness_count, 3, "Expected 3 fitness evaluations");
        check_equal(tournament_count, 3, "Expected 3 tournaments");
        check_equal(prior_best_count, 3, "Expected 3 prior best copies");
      elsif run("test_prior_best_preservation") then
        -- Test prior best model preservation with different intervals

        -- Set configuration parameters
        config.population_size_exp    <= to_unsigned(3, config.population_size_exp'length);    -- Population of 8
        config.model_history_size     <= to_unsigned(4, config.model_history_size'length);
        config.reference_count        <= to_unsigned(0, config.reference_count'length);
        config.model_history_interval <= to_unsigned(2, config.model_history_interval'length); -- Save every 2 generations
        config.max_gen                <= to_unsigned(5, config.max_gen'length);                -- Run for 5 generations
        config.run_until_stop_cmd     <= false;                                                -- Stop after max_gen
        config.seed                   <= (others => '0');                                      -- Set seed for reproducibility

        -- Start the GA
        wait until rising_edge(clk);
        go <= true;
        wait until rising_edge(clk);
        go <= false;

        -- Wait for completion - increase timeout for this test
        for i in 1 to 2000 loop
          if done then
            exit;
          end if;
          wait for CLK_100MHZ_PERIOD;
        end loop;

        check(done, "GA should have completed after max_gen");

        -- Wait a bit more to ensure all counts are registered
        wait for 10 * CLK_100MHZ_PERIOD;

        -- With interval=2 and 5 generations, expect 2 or 3 copies (depends on how counting works)
        check(prior_best_count >= 2 and prior_best_count <= 3,
              "Expected 2 or 3 prior best copies with interval=2 and 5 generations, got " &
              integer'image(prior_best_count));
      elsif run("test_generation_completion") then
        -- Test GA stops after reaching max_gen

        -- Set configuration parameters
        config.population_size_exp    <= to_unsigned(2, config.population_size_exp'length); -- Small population
        config.model_history_size     <= to_unsigned(1, config.model_history_size'length);
        config.reference_count        <= to_unsigned(0, config.reference_count'length);
        config.model_history_interval <= to_unsigned(1, config.model_history_interval'length);
        config.max_gen                <= to_unsigned(2, config.max_gen'length);             -- Run for 2 generations
        config.run_until_stop_cmd     <= false;                                             -- Stop after max_gen
        config.seed                   <= (others => '0');                                   -- Set seed for reproducibility

        -- Start the GA
        wait until rising_edge(clk);
        go <= true;
        wait until rising_edge(clk);
        go <= false;

        -- Wait for completion - increase timeout for this test
        for i in 1 to 1000 loop
          if done then
            exit;
          end if;
          wait for CLK_100MHZ_PERIOD;
        end loop;

        check(done, "GA should have completed after max_gen");

        -- Verify we ran the expected number of generations
        check_equal(fitness_count, 2, "Expected exactly 2 generations");

        -- Save current fitness count
        fitness_at_pause := fitness_count;

        -- Reset counters to check no more activity
        wait until rising_edge(clk);
        reset_counters <= true;
        wait until rising_edge(clk);
        reset_counters <= false;

        -- Wait some time
        wait for 50 * CLK_100MHZ_PERIOD;

        -- Verify GA doesn't continue running
        check_equal(fitness_count, 0, "GA should not run additional generations after completion");
      elsif run("test_continuous_run") then
        -- Test GA continues running in continuous mode

        -- Set configuration parameters
        config.population_size_exp    <= to_unsigned(2, config.population_size_exp'length);
        config.model_history_size     <= to_unsigned(1, config.model_history_size'length);
        config.reference_count        <= to_unsigned(0, config.reference_count'length);
        config.model_history_interval <= to_unsigned(1, config.model_history_interval'length);
        config.max_gen                <= to_unsigned(10, config.max_gen'length);
        config.run_until_stop_cmd     <= true;            -- Run until stopped
        config.seed                   <= (others => '0'); -- Set seed for reproducibility

        -- Start the GA
        wait until rising_edge(clk);
        go <= true;
        wait until rising_edge(clk);
        go <= false;

        -- Wait until we have at least 3 fitness cycles
        for i in 1 to 1000 loop
          if fitness_count >= 3 then
            exit;
          end if;
          wait for CLK_100MHZ_PERIOD;
        end loop;

        check(fitness_count >= 3, "GA should run continuously in continuous mode");

        -- Send pause
        wait until rising_edge(clk);
        pause <= true;

        -- Wait for done signal indicating successful pause
        for i in 1 to 1000 loop
          if done then
            exit;
          end if;
          wait for CLK_100MHZ_PERIOD;
        end loop;

        check(done, "GA should have paused and signaled done");

        wait until rising_edge(clk);
        pause <= false;
      elsif run("test_pause_resume") then
        -- Test pause and resume functionality

        -- Set configuration parameters
        config.population_size_exp    <= to_unsigned(2, config.population_size_exp'length);
        config.model_history_size     <= to_unsigned(1, config.model_history_size'length);
        config.reference_count        <= to_unsigned(0, config.reference_count'length);
        config.model_history_interval <= to_unsigned(2, config.model_history_interval'length);
        config.max_gen                <= to_unsigned(10, config.max_gen'length);
        config.run_until_stop_cmd     <= true;            -- Run until stopped
        config.seed                   <= (others => '0'); -- Set seed for reproducibility

        -- Start the GA
        wait until rising_edge(clk);
        go <= true;
        wait until rising_edge(clk);
        go <= false;

        -- Wait until we have at least 3 fitness cycles
        for i in 1 to 1000 loop
          if fitness_count >= 3 then
            exit;
          end if;
          wait for CLK_100MHZ_PERIOD;
        end loop;

        -- Save the fitness count for comparison
        fitness_at_pause := fitness_count;

        -- Send pause
        wait until rising_edge(clk);
        pause <= true;

        -- Wait for done signal indicating successful pause
        for i in 1 to 1000 loop
          if done then
            exit;
          end if;
          wait for CLK_100MHZ_PERIOD;
        end loop;

        check(done, "GA should have paused and signaled done");

        wait until rising_edge(clk);
        pause <= false;

        -- Check fitness count doesn't change while paused
        wait for 50 * CLK_100MHZ_PERIOD;
        check_equal(fitness_count, fitness_at_pause,
                    "Fitness count should not change while paused");

        -- Resume
        wait until rising_edge(clk);
        resume <= true;
        wait until rising_edge(clk);
        resume <= false;

        -- Verify it continues running
        for i in 1 to 1000 loop
          if fitness_count > fitness_at_pause then
            exit;
          end if;
          wait for CLK_100MHZ_PERIOD;
        end loop;

        check(fitness_count > fitness_at_pause,
              "Fitness count should increase after resume");
      end if;
    end loop;

    test_runner_cleanup(runner);
  end process;

end architecture tb_arch;
