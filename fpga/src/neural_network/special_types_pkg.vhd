library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;
use ieee.numeric_std.all;

-- Special Types Package
package special_types_pkg is

  type nn_weight_array is array (0 to 7) of std_logic_vector(31 downto 0);

end package special_types_pkg;
