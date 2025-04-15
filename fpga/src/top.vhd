-- top is the top level of the system.
-- it is very minimal: just hooks up external io to uart, then
-- hooks up uart to core. this keeps core easy to simulate.
library ieee;
use ieee.std_logic_1164.all;

entity top is
  port (
    -- uart rx
    i_rx_serial : in  std_logic;

    -- uart tx
    o_tx_serial : out std_logic;

    -- indicator led
    led_out : out std_logic
  );
end entity top;

architecture top_arch of top is

  constant G_CLKS_PER_BIT : integer := 868; -- 100MHz / 115200 baud = 868

  signal o_rx_dv   : std_logic;
  signal o_rx_byte : std_logic_vector(7 downto 0);

  signal i_tx_dv   : std_logic;
  signal i_tx_byte : std_logic_vector(7 downto 0);
  signal o_tx_done : std_logic;

  -- ps outputs
  signal pl_clk0_0    : std_logic;
  signal pl_resetn0_0 : std_logic;

begin

  -- instantiate the wrapper
  kv260_wrapper_inst : entity work.kv260_design_wrapper
    port map (
      pl_clk0      => pl_clk0_0,
      pl_resetn0_0 => pl_resetn0_0
    );

  uart_rx_ent : entity work.uart_rx
    generic map (
      G_CLKS_PER_BIT => G_CLKS_PER_BIT
    )
    port map (
      i_clk       => pl_clk0_0,
      i_rx_serial => i_rx_serial,
      o_rx_dv     => o_rx_dv,
      o_rx_byte   => o_rx_byte
    );

  uart_tx_ent : entity work.uart_tx
    generic map (
      G_CLKS_PER_BIT => G_CLKS_PER_BIT
    )
    port map (
      i_clk       => pl_clk0_0,
      i_tx_dv     => i_tx_dv,
      i_tx_byte   => i_tx_byte,
      o_tx_active => open,
      o_tx_serial => o_tx_serial,
      o_tx_done   => o_tx_done
    );

  core_ent : entity work.core
    port map (
      clk       => pl_clk0_0,
      o_rx_dv   => o_rx_dv,
      o_rx_byte => o_rx_byte,
      i_tx_dv   => i_tx_dv,
      i_tx_byte => i_tx_byte,
      o_tx_done => o_tx_done,
      led_out   => led_out
    );

end architecture top_arch;
