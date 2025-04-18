library ieee;

library vunit_lib;
context vunit_lib.vunit_context;

use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.fixed_pkg.all;
use ieee.fixed_float_types.all;

use work.nn_types.all;
use work.bram_types.all;
use work.game_types.all;
use work.decoder_funs.all;

entity tb_nn is
  generic (
    RUNNER_CFG : string
  );
end entity tb_nn;

architecture tb_arch of tb_nn is

  constant CLK_100MHZ_PERIOD : time := 10 ns;

  signal clk : std_logic := '0';

  -- inputs
  signal param       : param_t       := (others => '0');
  signal param_index : param_index_t := (others => '0');
  signal param_valid : boolean       := false;

  signal gs             : gamestate_t := default_gamestate_t;
  signal p1_perspective : boolean     := false; -- should this be set to true?
  signal go             : boolean     := false;

  -- outputs
  signal action : playerinput_t;
  signal done   : boolean;

begin

  -- Timeout after 125 us.
  test_runner_watchdog(runner, 125 us);

  clk <= not clk after CLK_100MHZ_PERIOD / 2;

  -- UUT instantiation
  uut : entity work.nn
    port map (
      clk            => clk,
      param          => param,
      param_index    => param_index,
      param_valid    => param_valid,
      gs             => gs,
      p1_perspective => p1_perspective,
      action         => action,
      go             => go,
      done           => done
    );

  -- Test process
  test_process : process is

    variable layer_index  : integer := 0;
    variable neuron_index : integer := 0;
    variable weight_index : integer := 0;

  begin
    test_runner_setup(runner, RUNNER_CFG);

    while test_suite loop
      wait until rising_edge(clk);

      for i in 0 to TOTAL_WEIGHTS - 1 loop
        param       <= "0001";
        param_valid <= true;
        param_index <= to_unsigned(i, param_index'length);
        wait until rising_edge(clk);
      end loop;

      if run("ones") then
        wait until rising_edge(clk);
        go <= true;
        wait until rising_edge(clk);
        go <= false;

        wait until done;
        -- neural net has positive inputs with positive weights,
        -- so all outputs should be high, meaning the actions should be true,
        -- because they are true when output > 0
        check_equal(action.left, true, "Left action incorrect output.");
        check_equal(action.right, true, "Right action incorrect output.");
        check_equal(action.jump, true, "Jump action incorrect output.");
      elsif run("ones_inv_dead_counter") then
        gs.p1.dead_timeout <= to_unsigned(1, gs.p1.dead_timeout'length);
        wait until rising_edge(clk);
        go                 <= true;
        wait until rising_edge(clk);
        go                 <= false;

        wait until done;
        wait until rising_edge(clk);

        -- we have positive 32 and negative 32 inputs, which should cancel
        -- each other out, resulting in 0, so our actions should be false.
        check_equal(action.left, false, "Left action incorrect output.");
        check_equal(action.right, false, "Right action incorrect output.");
        check_equal(action.jump, false, "Jump action incorrect output.");
      end if;

      wait until rising_edge(clk);
      wait until rising_edge(clk);
      wait until rising_edge(clk);
      wait until rising_edge(clk);
    end loop;

    test_runner_cleanup(runner);
  end process;

end architecture tb_arch;
