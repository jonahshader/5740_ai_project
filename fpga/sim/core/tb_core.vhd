library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library vunit_lib;
context vunit_lib.vunit_context;

entity tb_core is
  generic (
    RUNNER_CFG : string
  );
end entity tb_core;

architecture tb of tb_core is

  constant CLK_100MHZ_PERIOD : time := 10 ns;

  signal clk : std_logic := '0';

  -- valid signal for o_rx_byte. should be driven here.
  signal o_rx_dv : std_logic := '0';
  -- output of uart_rx. should be driven here.
  signal o_rx_byte : std_logic_vector(7 downto 0) := (others => '0');

  -- valid signal for i_tx_byte. should be read here.
  signal i_tx_dv : std_logic;
  -- input of uart_tx. should be read here.
  signal i_tx_byte : std_logic_vector(7 downto 0);
  -- indicates when the transmission out of PL is done. should be driven here.
  signal o_tx_done : std_logic := '0';

begin

  -- Timeout after 125 us.
  test_runner_watchdog(runner, 125 us);

  clk <= not clk after CLK_100MHZ_PERIOD / 2;

  core_ent : entity work.core
    port map (
      clk       => clk,
      o_rx_dv   => o_rx_dv,
      o_rx_byte => o_rx_byte,
      i_tx_dv   => i_tx_dv,
      i_tx_byte => i_tx_byte,
      o_tx_done => o_tx_done
    );

  main : process is
  begin
    test_runner_setup(runner, RUNNER_CFG);

    wait until rising_edge(clk);

    while test_suite loop
      if run("test_msg") then
        -- send the test message
        o_rx_byte <= x"05";
        o_rx_dv   <= '1';
        wait until rising_edge(clk);
        wait until i_tx_dv = '1';
        -- read message
        check_equal(i_tx_byte, x"68", "Core incorrect test message returned.");
        wait until rising_edge(clk);
      -- TODO: check more stuff?
      end if;
    end loop;

    test_runner_cleanup(runner);
  end process;

end architecture tb;
