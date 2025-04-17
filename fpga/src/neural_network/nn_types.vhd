library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

package nn_types is

  constant WEIGHT_BITS       : integer := 3;
  constant BIAS_BITS         : integer := 4;
  constant NEURON_DATA_WIDTH : integer := 10;

  constant WEIGHTS_PER_NEURON_EXP : integer := 5; -- 2 ** 5 = 32
  constant WEIGHTS_PER_NEURON     : integer := 2 ** WEIGHTS_PER_NEURON_EXP;
  -- refers to the number of weights per layer
  -- number of layers, excluding input layer
  constant LAYER_COUNT_EXP : integer := 2;
  constant LAYER_COUNT     : integer := 2 ** LAYER_COUNT_EXP;

  constant TOTAL_WEIGHTS : integer := (WEIGHTS_PER_NEURON ** 2) * LAYER_COUNT;
  constant TOTAL_BIAS    : integer := LAYER_COUNT * WEIGHTS_PER_NEURON;
  constant TOTAL_PARAMS  : integer := TOTAL_WEIGHTS + TOTAL_BIAS;

  -- all the types needed for a neuron
  subtype weight_t is signed(WEIGHT_BITS - 1 downto 0); -- Making the wights signed makes the bit shift easier
  type    weights_t is array (0 to WEIGHTS_PER_NEURON - 1) of weight_t;
  function default_weights_t return weights_t;
  subtype bias_t is signed(BIAS_BITS - 1 downto 0);
  subtype neuron_logit_t is signed(NEURON_DATA_WIDTH - 1 downto 0);
  type    neuron_logits_t is array(0 to WEIGHTS_PER_NEURON - 1) of neuron_logit_t;
  function default_neuron_logits_t return neuron_logits_t;
  subtype post_mult_t is signed(NEURON_DATA_WIDTH downto 0);

  -- all the parameters needed for a neuron
  type neuron_t is record
    weights : weights_t;
    bias    : bias_t;
  end record neuron_t;

  function weight_mult(data : neuron_logit_t; weight : weight_t) return post_mult_t;
  function neuron_forward(neuron : neuron_t; logits : neuron_logits_t; activate : boolean) return neuron_logit_t;

  type neurons_t is array (0 to WEIGHTS_PER_NEURON - 1) of neuron_t;
  function default_neurons_t return neurons_t;

  type layers_t is array (0 to LAYER_COUNT - 1) of neurons_t;
  function default_layers_t return layers_t;

  function layer_forward(layer : neurons_t; logits : neuron_logits_t; activate : boolean) return neuron_logits_t;

end package nn_types;

-- everything in the body is for implementation

package body nn_types is

  function default_neuron_t return neuron_t is
    variable val : neuron_t := (weights => default_weights_t,
                                 bias => (others => '0'));
  begin
    return val;
  end function;

  function default_weights_t return weights_t is
    variable val : weights_t := (others => (others => '0'));
  begin
    return val;
  end function;

  function default_neurons_t return neurons_t is
    variable val : neurons_t := (others => default_neuron_t);
  begin
    return val;
  end function;

  function default_neuron_logits_t return neuron_logits_t is
    variable val : neuron_logits_t := (others => (others => '0'));
  begin
    return val;
  end function;

  function layer_forward(layer : neurons_t; logits : neuron_logits_t; activate : boolean) return neuron_logits_t is
    variable out_logits : neuron_logits_t := (others => (others => '0'));
  begin
    for i in 0 to WEIGHTS_PER_NEURON - 1 loop
      out_logits(i) := neuron_forward(layer(i), logits, activate);
    end loop;

    return out_logits;
  end function;

  function weight_mult(data : neuron_logit_t; weight : weight_t) return post_mult_t is
    variable post_mult : post_mult_t := resize(data, NEURON_DATA_WIDTH + 1);
  begin
    case weight is
      when to_signed(-2, weight'length) =>
        post_mult := -shift_left(post_mult, 1);
      when to_signed(-1, weight'length) =>
        post_mult := -post_mult;
      when to_signed(2, weight'length) =>
        post_mult := shift_left(post_mult, 1);
      when others =>
        post_mult := to_signed(0, post_mult'length);
    end case;

    return post_mult;
  end function;

  function neuron_forward(neuron : neuron_t; logits : neuron_logits_t; activate : boolean) return neuron_logit_t is
    -- +1 for the weight operation's shift_left,
    -- +1 for bias
    -- +NEURON_DATA_WIDTH, the initial input size
    -- +WEIGHTS_PER_NEURON_EXP, the number of addition stages,
    --    where each sum results in an output with +1 bit
    -- -1 because vhdl is inclusive
    variable sum   : signed(2 + NEURON_DATA_WIDTH + WEIGHTS_PER_NEURON_EXP - 1 downto 0) := (others => '0');
    variable logit : neuron_logit_t; -- scaled output

    constant SUM_TO_LOGIT_SHIFT : integer := sum'length - NEURON_DATA_WIDTH - 3;
  begin
    for i in 0 to WEIGHTS_PER_NEURON - 1 loop
      sum := sum + weight_mult(logits(i), neuron.weights(i));
    end loop;

    if activate and sum < 0 then
      sum := to_signed(0, sum'length);
    end if;

    -- resize to fit into logit
    -- 001010100
    -- 0010
    -- 1111
    -- TODO: this is effectively shifting right, which is dividing.
    -- we should also try truncating the upper bits by saturating,
    -- or a mix of both (truncate some of the bottom, saturate some of the top).
    -- Joey says this is so ill
    --------" the illist of the illest"

    -- TODO: try other shift config.
    -- logit := sum(sum'length - 1 downto sum'length - NEURON_DATA_WIDTH);
    -- logit := sum(sum'length - 1 - 3 downto sum'length - NEURON_DATA_WIDTH - 3);

    -- 100110101
    -- 011001011
    -- 0..0010..
    -- 1..1101..

    -- 1..1101..
    -- 11111

    -- logit := sum(sum'length - 1) & sum(sum'length - 1 - 3 - 1 downto sum'length - NEURON_DATA_WIDTH - 3);

    logit := resize(
        shift_right(sum, SUM_TO_LOGIT_SHIFT),
        NEURON_DATA_WIDTH
      );
    return logit;
  end function;

  function default_layers_t return layers_t is
    variable val : layers_t := (others => default_neurons_t);
  begin
    return val;
  end function;

end package body nn_types;
