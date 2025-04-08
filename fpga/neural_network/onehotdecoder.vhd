library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity onehotdecoder is
  generic (
    INPUT_WIDTH  : integer := 8; -- Width of the one-hot input vector
    OUTPUT_WIDTH : integer := 8  -- Number of neural networks to select from
  );
  port (
    clk        : in std_logic;
    reset      : in std_logic;
    one_hot_in : in std_logic_vector(INPUT_WIDTH - 1 downto 0);
    nn_select  : out std_logic_vector(OUTPUT_WIDTH - 1 downto 0)
  );
end entity onehotdecoder;

architecture behavioral of onehotdecoder is

begin

  process (clk, reset) is
  begin
    if reset = '1' then
      nn_select <= (others => '0');
    elsif rising_edge(clk) then
      nn_select <= (others => '0');  -- Default: no neural network selected

      -- Decode the one-hot input
      for i in 0 to INPUT_WIDTH - 1 loop
        if one_hot_in(i) = '1' then
          -- If this index is within our output range
          if i < OUTPUT_WIDTH then
            nn_select(i) <= '1';
          end if;

          -- Exit the loop since we found the '1' (one-hot encoding)
          exit;
        end if;
      end loop;
    end if;
  end process;

end architecture behavioral;
