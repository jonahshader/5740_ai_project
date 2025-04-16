library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;
use ieee.numeric_std.all;

-- inputs will be of
-- type weights is array (natural range <>, natural range <>) of std_logic;

entity neuron is
  generic (
    NUM_INPUTS        : integer := 16; -- Configurable number of inputs
    DATA_WIDTH        : integer := 32; -- Configurable data width
    DATA_WEIGHT_WIDTH : integer := 3   -- size of the weights
  );
  port (
    weights : in weights(NUM_INPUTS - 1 downto 0, DATA_WEIGHT_WIDTH downto 0);       -- Packed weights
    inputs  : in std_logic_vector(NUM_INPUTS - 1 downto 0, DATA_WIDTH - 1 downto 0); -- Packed inputs
    bias    : in std_logic_vector(DATA_WIDTH - 1 downto 0);                          -- Bias term
    output  : out std_logic_vector(DATA_WIDTH - 1 downto 0)                          -- Neuron output
  );
end entity neuron;

architecture behavioral of neuron is

  component regiseter is
    generic (
      N : integer := DATA_WIDTH
    );
    port (
      clock  : in std_logic;
      resetn : in std_logic;
      e      : in std_logic; -- sclr: Synchronous clear
      d      : in std_logic_vector(DATA_WIDTH downto 0);
      q      : out std_logic_vector(DATA_WIDTH downto 0)
    );
  end component;

  signal sum_result             : std_logic_vector(DATA_WIDTH - 1 downto 0);
  signal weighted_inputs        : weights(NUM_INPUTS - 1 downto 0, DATA_WIDTH - 1 downto 0);
  signal signed_weighted_inputs : weights(NUM_INPUTS - 1 downto 0, DATA_WIDTH - 1 downto 0);
  signal accumulate             : std_logic_vector(2 * DATA_WIDTH - 1 downto 0);

begin

  weight_inputs : process (weights, inputs) is
  begin
    for i in 0 to NUM_INPUTS - 1 loop
      with weights(i)(1 downto 0) select weighted_inputs(i) <=
        inputs(i) & '0' when "10",
        inputs(i) when "01",
        (others => '0') when others;
    end loop;
  end process;

  sign_weighted_inputs : process (weights, weighted_inputs) is
  begin
    for i in 0 to NUM_INPUTS - 1 loop
      if weights(i)(DATA_WEIGHT_WIDTH - 1) = '1' then
        signed_weighted_inputs(i) <= weighted_inputs(i) & '0'; -- Negative weight
      else
        signed_weighted_inputs(i) <= weighted_inputs(i);       -- Positive weight
      end if;
    end loop;
  end process;

  accumulate_process : process (signed_weighted_inputs, bias) is
  begin
    accumulate <= (others => '0');
    for i in 0 to NUM_INPUTS - 1 loop
      accumulate <= accumulate + signed_weighted_inputs(i);
    end loop;
    -- Add bias with proper bit width
    accumulate <= accumulate + bias;
  end process;

  activation_process : process (accumulate) is
  begin
    -- ReLU: max(0, x)
    if accumulate(DATA_WIDTH - 1) = '1' then
      sum_result <= (others => '0');
    else
      sum_result <= accumulate;
    end if;
  end process;

  output <= sum_result;

end architecture behavioral;
