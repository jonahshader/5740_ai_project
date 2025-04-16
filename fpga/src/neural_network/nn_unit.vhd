library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;
use ieee.numeric_std.all;

-- Neural Network Unit
-- This module implements a simple feedforward neural network with two layers.
-- The first layer has PARAM_WIDTH inputs and NEURONS_PER_LAYER neurons.
-- The second layer has NEURONS_PER_LAYER inputs and NEURONS_PER_LAYER neurons.
-- The output is a single value derived from the first neuron's output in the second layer.
-- The architecture is parameterized to allow for different configurations of neurons and data widths.
-- This all might need to be changed once Joey sets up the LUTs for the weights and biases.

entity nn_unit is
  generic (
    NEURONS_PER_LAYER : integer := 32; -- Number of neurons per layer
    NUM_LAYERS        : integer := 2;  -- Number of layers in the network
    PARAM_WIDTH       : integer := 10; -- Width of parameters input
    DATA_WIDTH        : integer := 32; -- Width of data (weights, outputs, etc.)
    RESULT_WIDTH      : integer := 16  -- Width of the output result
  );
  port (
    clk              : in std_logic;
    rst              : in std_logic;
    enable           : in std_logic;                                  -- Enable processing
    parameters       : in std_logic_vector(PARAM_WIDTH - 1 downto 0); -- Parameters input (was game_state)
    parameters_valid : in std_logic;                                  -- Indicates new parameters are valid

    -- Weights for all neurons in all layers
    -- Layer 1: Each neuron has PARAM_WIDTH inputs
    -- Layer 2: Each neuron has NEURONS_PER_LAYER inputs
    weights : in std_logic_vector((PARAM_WIDTH * NEURONS_PER_LAYER +
                                      NEURONS_PER_LAYER * NEURONS_PER_LAYER) *
                                      DATA_WIDTH - 1 downto 0);

    -- Biases for all neurons
    biases : in std_logic_vector(NEURONS_PER_LAYER * NUM_LAYERS *
                                      DATA_WIDTH - 1 downto 0);

    -- Output result
    result : out std_logic_vector(RESULT_WIDTH - 1 downto 0);

    -- Processing complete flag
    done : out std_logic
  );
end entity nn_unit;

architecture structural of nn_unit is

  -- Component declaration for neuron
  component neuron is
    generic (
      NUM_INPUTS : integer;
      DATA_WIDTH : integer
    );
    port (
      clk     : in std_logic;
      rst     : in std_logic;
      weights : in std_logic_vector(NUM_INPUTS * DATA_WIDTH - 1 downto 0);
      inputs  : in std_logic_vector(NUM_INPUTS * DATA_WIDTH - 1 downto 0);
      bias    : in std_logic_vector(DATA_WIDTH - 1 downto 0);
      output  : out std_logic_vector(DATA_WIDTH - 1 downto 0)
    );
  end component neuron;

  -- Type definitions for signals
  type layer1_outputs_type is array (0 to NEURONS_PER_LAYER - 1) of std_logic_vector(DATA_WIDTH - 1 downto 0);
  type layer2_outputs_type is array (0 to NEURONS_PER_LAYER - 1) of std_logic_vector(DATA_WIDTH - 1 downto 0);

  -- Signals for neuron outputs
  signal layer1_outputs : layer1_outputs_type;
  signal layer2_outputs : layer2_outputs_type;

  -- Signal for extended parameters (to match neuron input width)
  signal extended_parameters : std_logic_vector(PARAM_WIDTH * DATA_WIDTH - 1 downto 0);

  -- Packed layer 1 outputs for layer 2 inputs
  signal layer1_outputs_packed : std_logic_vector(NEURONS_PER_LAYER * DATA_WIDTH - 1 downto 0);

  -- Parameter storage
  signal params_stored : std_logic_vector(PARAM_WIDTH - 1 downto 0);

begin

  -- Extend parameters for neuron inputs
  process (params_stored) is
  begin
    extended_parameters <= (others => '0');

    for i in 0 to PARAM_WIDTH - 1 loop
      if params_stored(i) = '1' then
        extended_parameters((i + 1) * DATA_WIDTH - 1 downto i * DATA_WIDTH) <= (0 => '1', others => '0');
      else
        extended_parameters((i + 1) * DATA_WIDTH - 1 downto i * DATA_WIDTH) <= (others => '0');
      end if;
    end loop;
  end process;

  -- Store parameters when valid
  process (clk) is
  begin
    if rising_edge(clk) then
      if rst = '1' then
        params_stored <= (others => '0');
      elsif parameters_valid = '1' then
        params_stored <= parameters;
      end if;
    end if;
  end process;

  -- Generate first layer of neurons
  gen_layer1 : for i in 0 to NEURONS_PER_LAYER - 1 generate
    neuron_layer1 : component neuron
      generic map (
        NUM_INPUTS => PARAM_WIDTH,
        DATA_WIDTH => DATA_WIDTH
      )
      port map (
        clk     => clk,
        rst     => rst,
        weights => weights((i + 1) * PARAM_WIDTH * DATA_WIDTH - 1 downto i * PARAM_WIDTH * DATA_WIDTH),
        inputs  => extended_parameters,
        bias    => biases((i + 1) * DATA_WIDTH - 1 downto i * DATA_WIDTH),
        output  => layer1_outputs(i)
      );
  end generate gen_layer1;

  -- Pack layer 1 outputs into a single vector for layer 2 inputs
  process (layer1_outputs) is
  begin
    for i in 0 to NEURONS_PER_LAYER - 1 loop
      layer1_outputs_packed((i + 1) * DATA_WIDTH - 1 downto i * DATA_WIDTH) <= layer1_outputs(i);
    end loop;
  end process;

  -- Generate second layer of neurons
  gen_layer2 : for i in 0 to NEURONS_PER_LAYER - 1 generate
    neuron_layer2 : component neuron
      generic map (
        NUM_INPUTS => NEURONS_PER_LAYER,
        DATA_WIDTH => DATA_WIDTH
      )
      port map (
        clk     => clk,
        rst     => rst,
        weights => weights(PARAM_WIDTH * NEURONS_PER_LAYER * DATA_WIDTH +
                          (i + 1) * NEURONS_PER_LAYER * DATA_WIDTH - 1 downto
                          PARAM_WIDTH * NEURONS_PER_LAYER * DATA_WIDTH +
                          i * NEURONS_PER_LAYER * DATA_WIDTH),
        inputs  => layer1_outputs_packed,
        bias    => biases(NEURONS_PER_LAYER * DATA_WIDTH + (i + 1) * DATA_WIDTH - 1 downto
                         NEURONS_PER_LAYER * DATA_WIDTH + i * DATA_WIDTH),
        output  => layer2_outputs(i)
      );
  end generate gen_layer2;

  -- Process to manage computation and output
  process (clk) is
  begin
    if rising_edge(clk) then
      if rst = '1' then
        result <= (others => '0');
        done   <= '0';
      elsif enable = '1' then
        -- Use the first neuron's output from layer 2 as the result
        -- Convert/resize from DATA_WIDTH to RESULT_WIDTH
        result <= layer2_outputs(0)(RESULT_WIDTH - 1 downto 0);
        done   <= '1';
      else
        done <= '0';
      end if;
    end if;
  end process;

end architecture structural;
