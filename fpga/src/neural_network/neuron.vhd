library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;
use ieee.numeric_std.all;

entity neuron is
  generic (
    -- Figure out fixed point repersentation
    -- Coordinates will be unsigned chars
    -- Velocity is a fixed point number
    NUM_INPUTS        : integer := 8; -- Configurable number of inputs
    DATA_WEIGHT_WIDTH : integer := 3; -- size of the weights
    DATA_WEIGHT_RADIX : integer := 0;
    DATA_WIDTH        : integer := 32 -- Configurable data width
  -- RADIX_POINT: integer := 4; -- Radix point is where the decimal point is in the number
  -- NN_TYPE : STRING := "PENTARY"
  );

  -- Inputs are in fixed point singed?
  -- weights are sign and value because this keeps shifting and mutation simple
  port (
    clk     : in std_logic;                                                     -- Clock signal
    rst     : in std_logic;                                                     -- Reset signal
    weights : in std_logic_vector(NUM_INPUTS * DATA_WEIGHT_WIDTH - 1 downto 0); -- Packed weights
    inputs  : in std_logic_vector(NUM_INPUTS * DATA_WIDTH - 1 downto 0);        -- Packed inputs
    bias    : in std_logic_vector(DATA_WIDTH - 1 downto 0);                     -- Bias term
    output  : out std_logic_vector(DATA_WIDTH - 1 downto 0)                     -- Neuron output
  );
end entity neuron;

architecture behavioral of neuron is

  -- Internal signals
  type input_array_type is array (0 to NUM_INPUTS - 1) of std_logic_vector(DATA_WIDTH - 1 downto 0);
  type weight_array_type is array (0 to NUM_INPUTS - 1) of std_logic_vector(DATA_WEIGHT_RADIX - 1 downto 0);

  signal input_array  : input_array_type;
  signal weight_array : weight_array_type;

  signal sum_result : std_logic_vector(DATA_WIDTH - 1 downto 0);
  -- accum_temp is larger than needed to keep the NN easily changed from pentary to normal
  signal accumulate : std_logic_vector(2 * DATA_WIDTH - 1 downto 0);

begin

  -- Process to unpack the inputs and weights
  unpack_process : process (weights, inputs) is
  begin
    for i in 0 to NUM_INPUTS - 1 loop
      weight_array(i) <= weights((i + 1) * DATA_WEIGHT_WIDTH - 1 downto i * DATA_WEIGHT_WIDTH);
      input_array(i)  <= inputs((i + 1) * DATA_WIDTH - 1 downto i * DATA_WIDTH);
    end loop;
  end process;

  -- Process to compute the weighted sum
  compute_process : process (clk, rst) is
    variable accumulate_temp : std_logic_vector(2 * DATA_WIDTH - 1 downto 0);
    variable product         : std_logic_vector(2 * DATA_WIDTH - 1 downto 0);
    variable extended_bias   : std_logic_vector(2 * DATA_WIDTH - 1 downto 0);

  begin
    if rst = '1' then
      accumulate_temp := (others => '0');
    elsif rising_edge(clk) then
      -- Initialize accumulator
      accumulate_temp := (others => '0');

      -- Sign-extend the bias to match accumulator width
      extended_bias                          := (others => bias(DATA_WIDTH - 1)); -- Copy sign bit to upper bits
      extended_bias(DATA_WIDTH - 1 downto 0) := bias;                             -- Copy original bias bits

      -- Multiply and accumulate - but witha a bit shift

      for i in 0 to NUM_INPUTS - 1 loop
        -- Probobly a question for jonah do you like the sll and srl operators
        -- does the multiplication for 0, 1, 2
        if weight_array(i)(1 downto  0) = "10" then
          product := input_array & '0';
        elsif weight_array(i)(1 downto 0) + "01" then
          product := input_array;
        else
          product := (others => '0');
        end if;

        -- double check we can assign product to product
        -- invert the number based on the sign
        if weight_array(i)(2) = "1" then
          product := product * (-1);
        end if;
      end loop;

      accumulate_temp := accumulate_temp + product;

      -- Add bias with proper bit width
      accumulate_temp := accumulate_temp + extended_bias;

      -- Store result
      accumulate <= accumulate_temp;
    end if;
  end process;

  -- Extract proper bits for the sum
  sum_result <= accumulate(DATA_WIDTH - 1 downto 0); -- this needs to be updated to match our fp size

  -- Simple ReLU activation function implemented directly
  activation_process : process (sum_result) is
  begin
    -- ReLU: max(0, x)
    if sum_result(DATA_WIDTH - 1) = '1' then -- Check if negative (MSB = 1)
      output <= (others => '0');             -- Output 0 for negative inputs
    else
      output <= sum_result;                  -- Pass through for positive inputs
    end if;
  end process;

end architecture behavioral;
