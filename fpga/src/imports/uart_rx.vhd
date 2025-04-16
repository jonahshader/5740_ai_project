----------------------------------------------------------------------
-- File Downloaded from http://www.nandland.com
----------------------------------------------------------------------
-- This file contains the UART Receiver.  This receiver is able to
-- receive 8 bits of serial data, one start bit, one stop bit,
-- and no parity bit.  When receive is complete o_rx_dv will be
-- driven high for one clock cycle.
--
-- Set Generic g_CLKS_PER_BIT as follows:
-- g_CLKS_PER_BIT = (Frequency of i_Clk)/(Frequency of UART)
-- Example: 10 MHz Clock, 115200 baud UART
-- (10000000)/(115200) = 87
--
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity uart_rx is
  generic (
    G_CLKS_PER_BIT : integer := 115     -- Needs to be set correctly
  );
  port (
    i_clk       : in  std_logic;
    i_rx_serial : in  std_logic;
    o_rx_dv     : out std_logic;
    o_rx_byte   : out std_logic_vector(7 downto 0)
  );
end entity uart_rx;

architecture rtl of uart_rx is

  type   t_sm_main is (
    S_IDLE, S_RX_START_BIT, S_RX_DATA_BITS,
    S_RX_STOP_BIT, S_CLEANUP
  );
  signal r_sm_main : t_sm_main := S_IDLE;

  signal r_rx_data_r : std_logic := '0';
  signal r_rx_data   : std_logic := '0';

  signal r_clk_count : integer range 0 to G_CLKS_PER_BIT - 1 := 0;
  signal r_bit_index : integer range 0 to 7                  := 0;  -- 8 Bits Total
  signal r_rx_byte   : std_logic_vector(7 downto 0)          := (others => '0');
  signal r_rx_dv     : std_logic                             := '0';

begin

  -- Purpose: Double-register the incoming data.
  -- This allows it to be used in the UART RX Clock Domain.
  -- (It removes problems caused by metastabiliy)
  p_sample : process (i_clk) is
  begin
    if rising_edge(i_clk) then
      r_rx_data_r <= i_rx_serial;
      r_rx_data   <= r_rx_data_r;
    end if;
  end process p_sample;

  -- Purpose: Control RX state machine
  p_uart_rx : process (i_clk) is
  begin
    if rising_edge(i_clk) then
      case r_sm_main is
        when S_IDLE =>
          r_rx_dv     <= '0';
          r_clk_count <= 0;
          r_bit_index <= 0;

          if r_rx_data = '0' then -- Start bit detected
            r_sm_main <= S_RX_START_BIT;
          else
            r_sm_main <= S_IDLE;
          end if;

        -- Check middle of start bit to make sure it's still low
        when S_RX_START_BIT =>
          if r_clk_count = (G_CLKS_PER_BIT - 1) / 2 then
            if r_rx_data = '0' then
              r_clk_count <= 0; -- reset counter since we found the middle
              r_sm_main   <= S_RX_DATA_BITS;
            else
              r_sm_main <= S_IDLE;
            end if;
          else
            r_clk_count <= r_clk_count + 1;
            r_sm_main   <= S_RX_START_BIT;
          end if;

        -- Wait g_CLKS_PER_BIT-1 clock cycles to sample serial data
        when S_RX_DATA_BITS =>
          if r_clk_count < G_CLKS_PER_BIT - 1 then
            r_clk_count <= r_clk_count + 1;
            r_sm_main   <= S_RX_DATA_BITS;
          else
            r_clk_count            <= 0;
            r_rx_byte(r_bit_index) <= r_rx_data;

            -- Check if we have sent out all bits
            if r_bit_index < 7 then
              r_bit_index <= r_bit_index + 1;
              r_sm_main   <= S_RX_DATA_BITS;
            else
              r_bit_index <= 0;
              r_sm_main   <= S_RX_STOP_BIT;
            end if;
          end if;

        -- Receive Stop bit. Stop bit = 1
        when S_RX_STOP_BIT =>
          -- Wait g_CLKS_PER_BIT-1 clock cycles for Stop bit to finish
          if r_clk_count < G_CLKS_PER_BIT - 1 then
            r_clk_count <= r_clk_count + 1;
            r_sm_main   <= S_RX_STOP_BIT;
          else
            r_rx_dv     <= '1';
            r_clk_count <= 0;
            r_sm_main   <= S_CLEANUP;
          end if;

        -- Stay here 1 clock
        when S_CLEANUP =>
          r_sm_main <= S_IDLE;
          r_rx_dv   <= '0';
        when others =>
          r_sm_main <= S_IDLE;

      end case;
    end if;
  end process p_uart_rx;

  o_rx_dv   <= r_rx_dv;
  o_rx_byte <= r_rx_byte;

end architecture rtl;
