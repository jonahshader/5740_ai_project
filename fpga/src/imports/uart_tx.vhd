----------------------------------------------------------------------
-- File Downloaded from http://www.nandland.com
----------------------------------------------------------------------
-- This file contains the UART Transmitter.  This transmitter is able
-- to transmit 8 bits of serial data, one start bit, one stop bit,
-- and no parity bit.  When transmit is complete o_TX_Done will be
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

entity uart_tx is
  generic (
    G_CLKS_PER_BIT : integer := 115     -- Needs to be set correctly
  );
  port (
    i_clk       : in  std_logic;
    i_tx_dv     : in  std_logic;
    i_tx_byte   : in  std_logic_vector(7 downto 0);
    o_tx_active : out std_logic;
    o_tx_serial : out std_logic;
    o_tx_done   : out std_logic
  );
end entity uart_tx;

architecture rtl of uart_tx is

  type   t_sm_main is (
    S_IDLE, S_TX_START_BIT, S_TX_DATA_BITS,
    S_TX_STOP_BIT, S_CLEANUP
  );
  signal r_sm_main : t_sm_main := S_IDLE;

  signal r_clk_count : integer range 0 to G_CLKS_PER_BIT - 1 := 0;
  signal r_bit_index : integer range 0 to 7                  := 0;  -- 8 Bits Total
  signal r_tx_data   : std_logic_vector(7 downto 0)          := (others => '0');
  signal r_tx_done   : std_logic                             := '0';

begin

  p_uart_tx : process (i_clk) is
  begin
    if rising_edge(i_clk) then

      case r_sm_main is
        when S_IDLE =>
          o_tx_active <= '0';
          o_tx_serial <= '1';         -- Drive Line High for Idle
          r_tx_done   <= '0';
          r_clk_count <= 0;
          r_bit_index <= 0;

          if i_tx_dv = '1' then
            r_tx_data <= i_tx_byte;
            r_sm_main <= S_TX_START_BIT;
          else
            r_sm_main <= S_IDLE;
          end if;

        -- Send out Start Bit. Start bit = 0
        when S_TX_START_BIT =>
          o_tx_active <= '1';
          o_tx_serial <= '0';

          -- Wait g_CLKS_PER_BIT-1 clock cycles for start bit to finish
          if r_clk_count < G_CLKS_PER_BIT - 1 then
            r_clk_count <= r_clk_count + 1;
            r_sm_main   <= S_TX_START_BIT;
          else
            r_clk_count <= 0;
            r_sm_main   <= S_TX_DATA_BITS;
          end if;

        -- Wait g_CLKS_PER_BIT-1 clock cycles for data bits to finish
        when S_TX_DATA_BITS =>
          o_tx_serial <= r_tx_data(r_bit_index);

          if r_clk_count < G_CLKS_PER_BIT - 1 then
            r_clk_count <= r_clk_count + 1;
            r_sm_main   <= S_TX_DATA_BITS;
          else
            r_clk_count <= 0;

            -- Check if we have sent out all bits
            if r_bit_index < 7 then
              r_bit_index <= r_bit_index + 1;
              r_sm_main   <= S_TX_DATA_BITS;
            else
              r_bit_index <= 0;
              r_sm_main   <= S_TX_STOP_BIT;
            end if;
          end if;

        -- Send out Stop bit. Stop bit = 1
        when S_TX_STOP_BIT =>
          o_tx_serial <= '1';

          -- Wait g_CLKS_PER_BIT-1 clock cycles for Stop bit to finish
          if r_clk_count < G_CLKS_PER_BIT - 1 then
            r_clk_count <= r_clk_count + 1;
            r_sm_main   <= S_TX_STOP_BIT;
          else
            r_tx_done   <= '1';
            r_clk_count <= 0;
            r_sm_main   <= S_CLEANUP;
          end if;

        -- Stay here 1 clock
        when S_CLEANUP =>
          o_tx_active <= '0';
          r_tx_done   <= '1';
          r_sm_main   <= S_IDLE;
        when others =>
          r_sm_main <= S_IDLE;

      end case;
    end if;
  end process p_uart_tx;

  o_tx_done <= r_tx_done;

end architecture rtl;
