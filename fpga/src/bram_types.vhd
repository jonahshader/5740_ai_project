library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.fixed_pkg.all;
use ieee.fixed_float_types.all;
use ieee.math_real.log2;
use ieee.math_real.ceil;

package bram_types is

  type    bram_command_t is (C_COPY_AND_MUTATE, C_READ);
  subtype bram_index_t is unsigned(7 downto 0);

  subtype param_t is std_logic_vector(3 downto 0);
  subtype param_index_t is unsigned(13 downto 0);

end package bram_types;

package body bram_types is

end package body bram_types;
