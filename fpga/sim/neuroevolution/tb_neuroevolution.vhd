library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library vunit_lib;
context vunit_lib.vunit_context;

entity tb_neuroevolution is
  generic (
    RUNNER_CFG : string
  );
end entity tb_neuroevolution;

architecture tb of tb_neuroevolution is

  constant CLK_100MHZ_PERIOD : time      := 10 ns;
  signal   clk               : std_logic := '0';

begin

end architecture tb;
