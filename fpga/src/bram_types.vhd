library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.math_real.log2;
use ieee.math_real.ceil;

package bram_types is

  constant NUM_BRAMS      : integer := 144;
  constant BRAM_WIDTH     : integer := 4;
  constant BRAM_DEPTH     : integer := 9216;
  constant BRAM_ADDR_BITS : integer := integer(ceil(log2(real(BRAM_DEPTH))));

  type    bram_command_t is (C_COPY_AND_MUTATE, C_READ);
  subtype bram_index_t is unsigned(7 downto 0);

  -- This is what is going into the NN
  -- the decoder will read it and pass it into the NN
  subtype param_t is std_logic_vector(3 downto 0);
  subtype param_index_t is unsigned(BRAM_ADDR_BITS - 1 downto 0);

end package bram_types;

package body bram_types is

end package body bram_types;
