library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;
use ieee.numeric_std.all;

--

entity neuron3 is
  generic (
    NUMBER_OF_INPUTS  : integer := 16; -- Number of inputs
    DATA_WIDTH        : integer := 32; -- Width of each input and weight
    DATA_WEIGHT_WIDTH : integer := 3;  -- Width of each weight
    );
    PORT (
    CLK               : in std_logic;
    RESET             : in std_logic;
    INPUTS            : in std_logic_vector(N - 1 downto 0);
    WEIGHTS           : in std_logic_vector(N * M - 1 downto 0);
    BIAS              : in std_logic_vector(M - 1 downto 0);
    OUTPUTS           : out std_logic_vector(M - 1 downto 0)
  );
end entity neuron3;
