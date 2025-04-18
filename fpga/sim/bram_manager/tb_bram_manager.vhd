library ieee;

library vunit_lib;
context vunit_lib.vunit_context;

use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.bram_types.all;
use work.nn_types.total_params;
use work.ga_types.mutation_rate_t;
use work.mutate_funs.all;

entity tb_bram_manager is
  generic (
    RUNNER_CFG : string
  );
end entity tb_bram_manager;

architecture tb_arch of tb_bram_manager is

  constant CLK_100MHZ_PERIOD : time      := 10 ns;
  signal   clk               : std_logic := '0';

  signal command       : bram_command_t                := C_COPY_AND_MUTATE;
  signal read_index    : bram_index_t                  := (others => '0');
  signal write_index   : bram_index_t                  := (others => '0');
  signal rng           : std_logic_vector(31 downto 0) := (others => '0');
  signal mutation_rate : mutation_rate_t               := (others => '0');

  signal param            : param_t;
  signal param_index      : param_index_t;
  signal param_valid_nn_1 : boolean;
  signal param_valid_nn_2 : boolean;

  signal go   : boolean := false;
  signal done : boolean;

begin

  bram_manager_ent : entity work.bram_manager
    port map (
      clk              => clk,
      command          => command,
      read_index       => read_index,
      write_index      => write_index,
      rng              => rng,
      mutation_rate    => mutation_rate,
      param            => param,
      param_index      => param_index,
      param_valid_nn_1 => param_valid_nn_1,
      param_valid_nn_2 => param_valid_nn_2,
      go               => go,
      done             => done
    );

  -- Timeout after 125 us.
  test_runner_watchdog(runner, 125 us);
  clk <= not clk after CLK_100MHZ_PERIOD / 2;

  test_process : process is
  begin
    test_runner_setup(runner, RUNNER_CFG);

    while test_suite loop
      wait until rising_edge(clk);
      if run("mutate_and_copy") then
        -- configure mutation rate and go
        -- (command already set to C_COPY_AND_MUTATE)
        mutation_rate <= to_unsigned(255, mutation_rate'length);
        go            <= true;
        wait until rising_edge(clk);
        go            <= false;

        -- wait until that operation is finished
        wait until done;
        wait until rising_edge(clk);

        -- now, read out that same BRAM
        command <= C_READ_TO_NN_1;
        go      <= true;
        wait until rising_edge(clk);
        go      <= false;

        -- wait until we get the first valid output
        wait until param_valid_nn_1;

        -- check all of them
        for i in 0 to TOTAL_PARAMS loop
          -- mutation rate is max, so every value should mutate.
          -- rng of 0 is interpreted as a mutation of -1,
          -- which is 1111 in std_logic_vector.
          check_equal(param, std_logic_vector'("1111"), "Param incorrect value.");
          check_equal(param_valid_nn_1, true, "Param incorrect validity.");
          check_equal(done, false, "Incorrect done.");
          wait until rising_edge(clk);
        end loop;
        check_equal(done, true, "Incorrect done.");
        check_equal(param_valid_nn_1, false, "Param incorrect validity.");
      end if;
    end loop;

    test_runner_cleanup(runner);
  end process;

end architecture tb_arch;
