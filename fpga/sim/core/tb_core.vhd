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
        -- valid signal goes high for once cycle
        o_rx_dv <= '1';
        wait until rising_edge(clk);
        o_rx_dv <= '0';

        -- wait for the reply
        wait until i_tx_dv = '1';
        -- TODO: should really drive o_tx_done high for a cycle here.

        -- read message
        check_equal(i_tx_byte, std_logic_vector'(x"68"), "Core incorrect test message returned.");

      -- TODO: check more stuff?
      elsif run("playerinput_msg") then
        -- send the PLAYER_INPUT_MSG
        o_rx_byte <= x"04";
        -- valid signal goes high for once cycle
        o_rx_dv <= '1';
        wait until rising_edge(clk);
        o_rx_dv <= '0';

        -- wait a cycle before sending the player input
        wait until rising_edge(clk);

        -- send the player input
        o_rx_byte <= "00000010"; -- go right
        -- valid signal goes high for once cycle
        o_rx_dv <= '1';
        wait until rising_edge(clk);
        o_rx_dv <= '0';

        -- we expect 18 transfers
        -- this test fails if we don't get the 18 transfers, because the watchdog will trip
        for i in 1 to 18 loop
          -- wait for outputs
          wait until i_tx_dv = '1';
          wait until rising_edge(clk);
          -- pulse done, indicating that we received this byte
          o_tx_done <= '1';
          wait until rising_edge(clk);
          o_tx_done <= '0';
        end loop;
      end if;

      wait until rising_edge(clk);
      wait until rising_edge(clk);
      wait until rising_edge(clk);
      wait until rising_edge(clk);
    end loop;

    test_runner_cleanup(runner);
    wait;
  end process;

end architecture tb;
