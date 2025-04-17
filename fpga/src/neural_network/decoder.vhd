library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.nn_types.all;
use work.bram_types.all;
use work.game_types.all;

-- (WEIGHTS_PER_NEURON ^ 2) * (LAYER_COUNT) + (LAYER_COUNT)

entity decoder is
  port (
    enable   : in std_logic;
    sel      : in std_logic_vector(BRAM_ADDR_BITS - 1 downto 0);
    layer
    weight
    neuron
  );
end entity decoder;

architecture rtl of decoder is

begin


  32bit = integer(ceil(log2(real(WEIGHTS_PER_LAYER))));
    0 - 11111 (0 to 31) - 32 bits

    weight = sel mod WEIGHTS_PER_LAYER
    nueuron = sel(max - 1 to 32bit) mod 32
    layer = sel(max - 1 to 32bit * 2) mod 4


    100000
    weight <= sel mod 32;


  demux : process (sel, enable) is
  begin
    data_out <= (others => '0');


end architecture rtl;
