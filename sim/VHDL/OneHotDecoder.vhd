library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity OneHotDecoder is
    generic (
        INPUT_WIDTH : integer := 8;  -- Width of the one-hot input vector
        OUTPUT_WIDTH : integer := 8  -- Number of neural networks to select from
    );
    port (
        clk         : in STD_LOGIC;
        reset       : in STD_LOGIC;
        one_hot_in  : in STD_LOGIC_VECTOR(INPUT_WIDTH-1 downto 0);
        nn_select   : out STD_LOGIC_VECTOR(OUTPUT_WIDTH-1 downto 0)
    );
end OneHotDecoder;

architecture Behavioral of OneHotDecoder is
begin
    process(clk, reset)
    begin
        if reset = '1' then
            nn_select <= (others => '0');
        elsif rising_edge(clk) then
            nn_select <= (others => '0');  -- Default: no neural network selected

            -- Decode the one-hot input
            for i in 0 to INPUT_WIDTH-1 loop
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
end Behavioral;
