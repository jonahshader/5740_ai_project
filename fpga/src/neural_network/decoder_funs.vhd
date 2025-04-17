library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.math_real.log2;
use ieee.math_real.ceil;

use work.nn_types.all;
use work.bram_types.all;

package decoder_funs is

  function decode_address(layers : layers_t; param : param_t; param_index : param_index_t) return layers_t;

-- if param_valid then
--    layers <= decode_addres(layers, param, param_index);
-- end if

end package decoder_funs;

package body decoder_funs is

  function decode_address(layers : layers_t; param : param_t; param_index : param_index_t) return layers_t is
    variable layers_new   : layers_t      := layers;
    variable index        : param_index_t := param_index;
    variable layer_index  : param_index_t;
    variable neuron_index : param_index_t;
    variable weight_index : param_index_t;

    variable bit_index : unsigned(3 downto 0) := (others => '0');
  begin
    if index < TOTAL_WEIGHTS then
      -- updating weight
      -- holy formatting...
      weight_index                                                                                    := param_index(WEIGHTS_PER_NEURON_EXP + to_integer(bit_index) - 1 downto to_integer(bit_index));
      bit_index                                                                                       := bit_index + WEIGHTS_PER_NEURON_EXP;
      neuron_index                                                                                    := param_index(WEIGHTS_PER_NEURON_EXP + to_integer(bit_index) - 1 downto to_integer(bit_index));
      bit_index                                                                                       := bit_index + WEIGHTS_PER_NEURON_EXP;
      layer_index                                                                                     := param_index(LAYER_COUNT_EXP + to_integer(bit_index) - 1 downto to_integer(bit_index));
      layers_new(to_integer(layer_index))(to_integer(neuron_index)).weights(to_integer(weight_index)) := signed(param(WEIGHT_BITS - 1 downto 0));
    else
      -- updating bias
      index := index - TOTAL_WEIGHTS;
    end if;
    -- layers_new(neuron_num)()
    return layers_new;
  end function;

end package body decoder_funs;
