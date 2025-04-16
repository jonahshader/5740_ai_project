library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.math_real.log2;
use ieee.math_real.ceil;

package nn_types is

  constant WEIGHT_BITS       : integer := 3;
  constant BIAS_BITS         : integer := 4;
  constant NEURON_DATA_WIDTH : integer := 10;

  constant WEIGHTS_PER_NEURON_EXP : integer := 5; -- 2 ** 5 = 32
  constant WEIGHTS_PER_NEURON     : integer := 2 ** WEIGHTS_PER_NEURON_EXP;

  -- all the types needed for a neuron
  subtype weight_t is signed(WEIGHT_BITS - 1 downto 0); -- Making the wights signed makes the bit shift easier
  type    weights_t is array (0 to WEIGHTS_PER_NEURON - 1) of weight_t;
  function default_weights_t return weights_t;
  subtype bias_t is signed(BIAS_BITS - 1 downto 0);
  subtype neuron_logit_t is signed(NEURON_DATA_WIDTH - 1 downto 0);

  -- all the parameters needed for a neuron
  type neuron_t is record
    weights : weights_t;
    bias    : bias_t;
    logit   : neuron_logit_t; -- acts as both the input and output register
  end record neuron_t;

  function neuron_forward(neuron : neuron_t; activate : boolean) return neuron_logit_t;

end package nn_types;

-- everything in the body is for implementation

package body nn_types is

  function default_neuron_t return neuron_t is
    variable val : neuron_t := (
                                 weights => default_weights_t,
                                bias    => (others => '0'),
                                 logit   => (others => '0')
                               );
  begin
    return val;
  end function;

  function default_weights_t return weights_t is
    variable val : weights_t := (others => (others => '0'));
  begin
    return val;
  end function;

  function neuron_forward(neuron : neuron_t; activate : boolean) return neuron_logit_t is
    constant
    variable sum :
  begin
  end function;

end package body nn_types;
