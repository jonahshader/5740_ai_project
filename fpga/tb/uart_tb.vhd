----------------------------------------------------------------------
-- File Downloaded from http://www.nandland.com
----------------------------------------------------------------------
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity uart_tb is
end entity uart_tb;

architecture behave of uart_tb is

  component uart_tx is
    generic (
      G_CLKS_PER_BIT : integer := 115 -- Needs to be set correctly
    );
    port (
      i_clk       : in  std_logic;
      i_tx_dv     : in  std_logic;
      i_tx_byte   : in  std_logic_vector(7 downto 0);
      o_tx_active : out std_logic;
      o_tx_serial : out std_logic;
      o_tx_done   : out std_logic
    );
  end component uart_tx;

  component uart_rx is
    generic (
      G_CLKS_PER_BIT : integer := 115 -- Needs to be set correctly
    );
    port (
      i_clk       : in  std_logic;
      i_rx_serial : in  std_logic;
      o_rx_dv     : out std_logic;
      o_rx_byte   : out std_logic_vector(7 downto 0)
    );
  end component uart_rx;

  -- Test Bench uses a 10 MHz Clock
  -- Want to interface to 115200 baud UART
  -- 10000000 / 115200 = 87 Clocks Per Bit.
  constant C_CLKS_PER_BIT : integer := 87;

  constant C_BIT_PERIOD : time := 8680 ns;

  signal r_clock     : std_logic                    := '0';
  signal r_tx_dv     : std_logic                    := '0';
  signal r_tx_byte   : std_logic_vector(7 downto 0) := (others => '0');
  signal w_tx_serial : std_logic;
  signal w_tx_done   : std_logic;
  signal w_rx_dv     : std_logic;
  signal w_rx_byte   : std_logic_vector(7 downto 0);
  signal r_rx_serial : std_logic                    := '1';

  -- Low-level byte-write
  procedure uart_write_byte (
    i_data_in       : in  std_logic_vector(7 downto 0);
    signal o_serial : out std_logic
  ) is
  begin
    -- Send Start Bit
    o_serial <= '0';
    wait for c_BIT_PERIOD;

    -- Send Data Byte
    for ii in 0 to 7 loop
      o_serial <= i_data_in(ii);
      wait for c_BIT_PERIOD;
    end loop; -- ii

    -- Send Stop Bit
    o_serial <= '1';
    wait for c_BIT_PERIOD;
  end procedure;

begin

  -- Instantiate UART transmitter
  uart_tx_inst : component uart_tx
    generic map (
      G_CLKS_PER_BIT => c_CLKS_PER_BIT
    )
    port map (
      i_clk       => r_clock,
      i_tx_dv     => r_tx_dv,
      i_tx_byte   => r_tx_byte,
      o_tx_active => open,
      o_tx_serial => w_tx_serial,
      o_tx_done   => w_tx_done
    );

  -- Instantiate UART Receiver
  uart_rx_inst : component uart_rx
    generic map (
      G_CLKS_PER_BIT => c_CLKS_PER_BIT
    )
    port map (
      i_clk       => r_clock,
      i_rx_serial => r_rx_serial,
      o_rx_dv     => w_rx_dv,
      o_rx_byte   => w_rx_byte
    );

  r_clock <= not r_clock after 50 ns;

  process is
  begin
    -- Tell the UART to send a command.
    wait until rising_edge(r_clock);
    wait until rising_edge(r_clock);
    r_tx_dv   <= '1';
    r_tx_byte <= x"AB";
    wait until rising_edge(r_clock);
    r_tx_dv   <= '0';
    wait until w_tx_done = '1';

    -- Send a command to the UART
    wait until rising_edge(r_clock);
    uart_write_byte(X"3F", r_rx_serial);
    wait until rising_edge(r_clock);

    -- Check that the correct command was received
    if w_rx_byte = x"3F" then
      report "Test Passed - Correct Byte Received"
        severity note;
    else
      report "Test Failed - Incorrect Byte Received"
        severity note;
    end if;

    assert false report "Tests Complete"
      severity failure;
  end process;

end architecture behave;
