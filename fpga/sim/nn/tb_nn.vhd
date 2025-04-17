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

  component nn is
    port (
      clk : in std_logic;

      -- bram io
      param       : in param_t;
      param_index : in param_index_t;
      param_valid : in boolean;

      -- nn io
      gs             : in gamestate_t;
      p1_perspective : in boolean;
      action         : out playerinput_t;
      go             : in boolean;
      done           : out boolean
    );
  end component nn;

  constant CLK_100MHZ_PERIOD : time := 10 ns;

  signal   clk        : std_logic := '0';
  constant CLK_PERIOD : time      := 10 ns;

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

  signal test_done : boolean := false;

begin

  -- Timeout after 125 us.
  test_runner_watchdog(runner, 125 us);

  clk <= not clk after CLK_100MHZ_PERIOD / 2;

  -- UUT instantiation
  uut : component nn
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
    -- set all the values to one
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
      elsif run("ones_dead_counter_1") then
        gs.p1.dead_timeout <= to_unsigned(1, gs.p1.dead_timeout'length);
        wait until rising_edge(clk);
        go                 <= true;
        wait until rising_edge(clk);
        go                 <= false;

        wait until done;
      end if;

      wait until rising_edge(clk);
      wait until rising_edge(clk);
      wait until rising_edge(clk);
      wait until rising_edge(clk);
    end loop;
  end process;

end architecture tb_arch;
