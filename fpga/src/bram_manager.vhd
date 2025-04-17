-- the bram manager houses all the brams, which store the
-- neural network parameters. the bram manager is responsible
-- for copying neural nets to other brams, copy & mutate,
-- and randomly initializing.

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.math_real.log2;
use ieee.math_real.ceil;

use work.bram_types.all;

entity bram_manager is
  generic (
    PARAMS : param_index_t
  );
  port (
    clk         : in std_logic;
    command     : in bram_command_t;
    read_index  : in bram_index_t;
    write_index : in bram_index_t;

    param       : out param_t       := (others => '0');
    param_index : out param_index_t := to_unsigned(0, 14);
    param_valid : out boolean       := false;

    go    : in boolean;
    ready : out boolean; -- able to accept a command
    done  : out boolean  -- done processing all commands
  );
end entity bram_manager;

architecture bram_manager_arch of bram_manager is

  type dout_b_arr_t is array (0 to NUM_BRAMS - 1) of param_t;

  type we_a_arr_t is array (0 to NUM_BRAMS - 1) of boolean;

  signal we_a_arr   : we_a_arr_t;
  signal dout_b_arr : dout_b_arr_t;

  subtype addr_t is unsigned(integer(ceil(log2(real(BRAM_DEPTH)))) - 1 downto 0);

  signal addr_a : addr_t := (others => '0');
  signal addr_b : addr_t := (others => '0');
  signal din_a  : param_t;

begin

  bram_gen : for i in 0 to NUM_BRAMS - 1 generate
    bram_inst : entity work.bram_sdp
      generic map (
        WIDTH => BRAM_WIDTH,
        DEPTH => BRAM_DEPTH
      )
      port map (
        -- port a (write only)
        clk_a  => clk,
        we_a   => we_a_arr(i),
        addr_a => addr_a,
        din_a  => din_a,
        -- port b (read only)
        clk_b  => clk,
        en_b   => true,
        addr_b => addr_b,
        dout_b => dout_b_arr(i)
      );
  end generate bram_gen;

end architecture bram_manager_arch;
