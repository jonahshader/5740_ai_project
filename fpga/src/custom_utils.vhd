library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.fixed_pkg.all;
use ieee.fixed_float_types.all;
use ieee.math_real.log2;
use ieee.math_real.ceil;
use work.game_types.all;

package custom_utils is

  function to_std_logic(b : boolean) return std_logic;

end package custom_utils;

package body custom_utils is

  function to_std_logic(b : boolean) return std_logic is
  begin
    if b then
      return '1';
    else
      return '0';
    end if;
  end function;

end package body custom_utils;
