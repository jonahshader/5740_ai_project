library ieee;

library vunit_lib;
context vunit_lib.vunit_context;

use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.bram_types.all;
use work.ga_types.all;
use work.custom_utils.all;

entity tb_victor_copy is
  generic (
    RUNNER_CFG : string
  );
end entity tb_victor_copy;

architecture tb_arch of tb_victor_copy is

  constant CLK_100MHZ_PERIOD : time      := 10 ns;
  signal   clk               : std_logic := '0';

  -- victor_copy inputs
  signal config        : ga_config_t           := default_ga_config_t;
  signal winner_counts : winner_counts_array_t := default_winner_counts_array_t;
  signal go            : boolean               := false;

  -- victor_copy outputs
  signal command         : bram_command_t;
  signal read_index      : bram_index_t;
  signal write_index     : bram_index_t;
  signal mutation_rate   : mutation_rate_t;
  signal bram_manager_go : boolean;
  signal done            : boolean;

  -- Signal to simulate bram_manager response
  signal bram_manager_done : boolean := true;

  -- For tracking copy operations
  signal copy_requested : boolean := false;
  signal copy_count     : integer := 0;

begin

  -- Instantiate the device under test
  dut : entity work.victor_copy
    port map (
      clk               => clk,
      config            => config,
      winner_counts     => winner_counts,
      command           => command,
      read_index        => read_index,
      write_index       => write_index,
      mutation_rate     => mutation_rate,
      bram_manager_go   => bram_manager_go,
      bram_manager_done => bram_manager_done,
      go                => go,
      done              => done
    );

  -- Timeout after 100 us
  test_runner_watchdog(runner, 100 us);
  clk <= not clk after CLK_100MHZ_PERIOD / 2;

  -- Simulate bram_manager response
  bram_manager_simulator : process is
  begin
    wait until bram_manager_go = true;

    -- Set the flag for the main process to increment the counter
    copy_requested <= true;

    -- Check that command is correct
    check(command = C_COPY_AND_MUTATE, "Expected COPY_AND_MUTATE command");

    -- Verify read_index points to a multi-victor and write_index to a non-victor
    check(winner_counts(to_integer(read_index)) >= 2,
          "read_index should point to a multi-victor (count>=2), but points to index " &
          integer'image(to_integer(read_index)) & " with count " &
          integer'image(to_integer(winner_counts(to_integer(read_index)))));

    check(winner_counts(to_integer(write_index)) = 0,
          "write_index should point to a non-victor (count=0), but points to index " &
          integer'image(to_integer(write_index)) & " with count " &
          integer'image(to_integer(winner_counts(to_integer(write_index)))));

    -- Verify mutation_rate matches the expected value from config
    check(mutation_rate = config.mutation_rates(to_integer(write_index)),
          "Mutation rate should match the config value for the write_index");

    -- Simulate BRAM manager execution time
    bram_manager_done <= false;
    wait for 5 * CLK_100MHZ_PERIOD;
    bram_manager_done <= true;
    wait until bram_manager_go = false;

    -- Reset the flag
    copy_requested <= false;
  end process;

  -- Process to count copy operations (single driver for copy_count)
  copy_counter_process : process is
  begin
    wait until rising_edge(clk);
    if copy_requested = true and copy_requested'event then
      copy_count <= copy_count + 1;
    end if;
  end process;

  test_process : process is

    variable pop_size            : integer;
    variable non_victor_count    : integer;
    variable multi_victor_extra  : integer;
    variable expected_copy_count : integer;

  begin
    test_runner_setup(runner, RUNNER_CFG);

    while test_suite loop
      if run("test_basic_distribution") then
        -- Configure population size to 8 (2^3)
        config.population_size_exp <= to_unsigned(3, config.population_size_exp'length);
        pop_size                   := 8;

        -- Configure mutation rates for the population
        for i in 0 to pop_size - 1 loop
          config.mutation_rates(i) <= to_unsigned(100 + i * 10, 8); -- Different rates for testing
        end loop;

        -- Reset winner counts
        winner_counts <= default_winner_counts_array_t;

        -- Set up winner distribution that sums to population size
        -- 4 non-victors (count=0), 2 single-victors (count=1), and 2 multi-victors (count>=2)
        winner_counts(0) <= to_unsigned(0, 8); -- Non-victor
        winner_counts(1) <= to_unsigned(0, 8); -- Non-victor
        winner_counts(2) <= to_unsigned(0, 8); -- Non-victor
        winner_counts(3) <= to_unsigned(0, 8); -- Non-victor
        winner_counts(4) <= to_unsigned(1, 8); -- Single victor
        winner_counts(5) <= to_unsigned(1, 8); -- Single victor
        winner_counts(6) <= to_unsigned(3, 8); -- Multi-victor with 3 wins
        winner_counts(7) <= to_unsigned(3, 8); -- Multi-victor with 3 wins

        -- Reset copy counter
        copy_count <= 0;

        -- Calculate expected number of copies
        non_victor_count    := 4;
        multi_victor_extra  := 4; -- Total extra wins (3-1)+(3-1) = 4
        expected_copy_count := minimum(non_victor_count, multi_victor_extra);

        wait for CLK_100MHZ_PERIOD;

        -- Start the process
        go <= true;
        wait for CLK_100MHZ_PERIOD;
        go <= false;

        -- Wait for completion
        wait until done;
        wait for CLK_100MHZ_PERIOD * 2;

        -- Verify the number of copy operations matches expectations
        check(copy_count = expected_copy_count,
              "Expected " & integer'image(expected_copy_count) &
              " copy operations, but got " & integer'image(copy_count));
      elsif run("test_minimal_multi_victors") then
        -- Test with just enough multi-victors to cover non-victors
        config.population_size_exp <= to_unsigned(3, config.population_size_exp'length);
        pop_size                   := 8;

        for i in 0 to pop_size - 1 loop
          config.mutation_rates(i) <= to_unsigned(50, 8);
        end loop;

        -- Reset winner counts
        winner_counts <= default_winner_counts_array_t;

        -- Distribution: 4 non-victors, 2 single-victors, 1 double-victor, 1 triple-victor
        -- Sum = 0+0+0+0+1+1+2+4 = 8 (population size)
        winner_counts(0) <= to_unsigned(0, 8); -- Non-victor
        winner_counts(1) <= to_unsigned(0, 8); -- Non-victor
        winner_counts(2) <= to_unsigned(0, 8); -- Non-victor
        winner_counts(3) <= to_unsigned(0, 8); -- Non-victor
        winner_counts(4) <= to_unsigned(1, 8); -- Single victor
        winner_counts(5) <= to_unsigned(1, 8); -- Single victor
        winner_counts(6) <= to_unsigned(2, 8); -- Double-victor
        winner_counts(7) <= to_unsigned(4, 8); -- Quad-victor

        -- Reset copy counter
        copy_count <= 0;

        -- Calculate expected copies
        non_victor_count    := 4;
        multi_victor_extra  := 4; -- Extra wins (2-1)+(4-1) = 4
        expected_copy_count := minimum(non_victor_count, multi_victor_extra);

        wait for CLK_100MHZ_PERIOD;

        -- Start the process
        go <= true;
        wait for CLK_100MHZ_PERIOD;
        go <= false;

        -- Wait for completion
        wait until done;
        wait for CLK_100MHZ_PERIOD * 2;

        -- Verify the number of copy operations
        check(copy_count = expected_copy_count,
              "Expected " & integer'image(expected_copy_count) &
              " copy operations, but got " & integer'image(copy_count));
      elsif run("test_no_multi_victors") then
        -- Test case where there are no multi-victors (should terminate quickly)
        config.population_size_exp <= to_unsigned(2, config.population_size_exp'length);
        pop_size                   := 4;

        -- Reset winner counts
        winner_counts <= default_winner_counts_array_t;

        -- Distribution: 1 non-victor, 3 single-victors
        -- Sum = 0+1+1+2 = 4 (population size)
        winner_counts(0) <= to_unsigned(0, 8); -- Non-victor
        winner_counts(1) <= to_unsigned(1, 8); -- Single victor
        winner_counts(2) <= to_unsigned(1, 8); -- Single victor
        winner_counts(3) <= to_unsigned(2, 8); -- Multi-victor

        -- Reset copy counter
        copy_count <= 0;

        -- We expect 1 copy since there is 1 non-victor and 1 multi-victor
        expected_copy_count := 1;

        wait for CLK_100MHZ_PERIOD;

        -- Start the process
        go <= true;
        wait for CLK_100MHZ_PERIOD;
        go <= false;

        -- Wait for completion
        wait until done;
        wait for CLK_100MHZ_PERIOD * 2;

        -- Verify copy operations
        check(copy_count = expected_copy_count,
              "Expected " & integer'image(expected_copy_count) &
              " copy operations with limited multi-victors");
      elsif run("test_no_non_victors") then
        -- Test case where there are no non-victors (should terminate quickly)
        config.population_size_exp <= to_unsigned(2, config.population_size_exp'length);
        pop_size                   := 4;

        -- Reset winner counts
        winner_counts <= default_winner_counts_array_t;

        -- Distribution: 0 non-victors, 2 single-victors, 1 double-victor
        -- Sum = 1+1+2+0 = 4 (population size)
        winner_counts(0) <= to_unsigned(1, 8); -- Single victor
        winner_counts(1) <= to_unsigned(1, 8); -- Single victor
        winner_counts(2) <= to_unsigned(2, 8); -- Multi-victor
        winner_counts(3) <= to_unsigned(0, 8); -- Non-victor

        -- Reset copy counter
        copy_count <= 0;

        -- We expect 1 copy since there is one non-victor
        expected_copy_count := 1;

        wait for CLK_100MHZ_PERIOD;

        -- Start the process
        go <= true;
        wait for CLK_100MHZ_PERIOD;
        go <= false;

        -- Wait for completion
        wait until done;
        wait for CLK_100MHZ_PERIOD * 2;

        -- Verify copy count
        check(copy_count = expected_copy_count,
              "Expected " & integer'image(expected_copy_count) &
              " copy operation for one non-victor");
      elsif run("test_more_non_victors_than_extra_wins") then
        -- Test when there are more non-victors than extra wins available
        config.population_size_exp <= to_unsigned(3, config.population_size_exp'length);
        pop_size                   := 8;

        -- Reset winner counts
        winner_counts <= default_winner_counts_array_t;

        -- Distribution: 6 non-victors, 0 single-victors, 1 with 8 wins
        -- Sum = 0+0+0+0+0+0+0+8 = 8 (population size)
        winner_counts(0) <= to_unsigned(0, 8); -- Non-victor
        winner_counts(1) <= to_unsigned(0, 8); -- Non-victor
        winner_counts(2) <= to_unsigned(0, 8); -- Non-victor
        winner_counts(3) <= to_unsigned(0, 8); -- Non-victor
        winner_counts(4) <= to_unsigned(0, 8); -- Non-victor
        winner_counts(5) <= to_unsigned(0, 8); -- Non-victor
        winner_counts(6) <= to_unsigned(0, 8); -- Non-victor
        winner_counts(7) <= to_unsigned(8, 8); -- Super-victor

        -- Reset copy counter
        copy_count <= 0;

        -- Calculate expected copies
        non_victor_count    := 7;
        multi_victor_extra  := 7; -- Extra wins (8-1) = 7
        expected_copy_count := minimum(non_victor_count, multi_victor_extra);

        wait for CLK_100MHZ_PERIOD;

        -- Start the process
        go <= true;
        wait for CLK_100MHZ_PERIOD;
        go <= false;

        -- Wait for completion
        wait until done;
        wait for CLK_100MHZ_PERIOD * 2;

        -- Verify the number of copy operations
        check(copy_count = expected_copy_count,
              "Expected " & integer'image(expected_copy_count) &
              " copy operations with limited multi-victors");
      end if;

      wait for CLK_100MHZ_PERIOD;
    end loop;

    test_runner_cleanup(runner);
  end process;

end architecture tb_arch;
