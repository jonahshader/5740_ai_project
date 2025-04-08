library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;
use IEEE.MATH_REAL.ALL;

-- Generic MUX for selecting one of multiple input vectors
-- I am not sure if this is the best way to do this, but it works.
-- I don't like that the selector is a vector, but it is easier to use. 
entity Generic_MUX is
    generic (
        NUM_INPUTS    : integer := 4;     -- Number of input vectors
        DATA_WIDTH    : integer := 8;     -- Width of each input/output vector
        -- Calculate required selector width based on number of inputs
        SEL_WIDTH     : integer := integer(ceil(log2(real(4))))  -- Default for 4 inputs
    );
    port (
        -- Input array implemented as a concatenated vector
        inputs      : in  STD_LOGIC_VECTOR((NUM_INPUTS * DATA_WIDTH) - 1 downto 0);
        selector    : in  STD_LOGIC_VECTOR(SEL_WIDTH - 1 downto 0);
        output      : out STD_LOGIC_VECTOR(DATA_WIDTH - 1 downto 0);

        -- Optional clock and enable signals (comment out if not needed)
        clk         : in  STD_LOGIC;
        enable      : in  STD_LOGIC
    );
end Generic_MUX;

architecture Behavioral of Generic_MUX is
    -- Array type for easier handling of input vectors
    type input_array is array (0 to NUM_INPUTS - 1) of STD_LOGIC_VECTOR(DATA_WIDTH - 1 downto 0);
    signal input_vectors : input_array;
begin
    -- Convert the flat input vector to an array for easier processing
    process(inputs)
    begin
        for i in 0 to NUM_INPUTS - 1 loop
            input_vectors(i) <= inputs(((i+1) * DATA_WIDTH) - 1 downto i * DATA_WIDTH);
        end loop;
    end process;

    -- Multiplexing process with optional clock synchronization
    process(clk, selector, input_vectors, enable)
        variable sel_index : integer range 0 to NUM_INPUTS - 1;
    begin
        -- Convert selector to integer
        sel_index := to_integer(unsigned(selector));

        -- Clocked version (synchronous)
        if rising_edge(clk) and enable = '1' then
            -- Check if selection is valid
            if sel_index < NUM_INPUTS then
                output <= input_vectors(sel_index);
            else
                -- For invalid selection, default to zeros or first input
                output <= (others => '0');
            end if;
        -- Combinational version (asynchronous, always active)
        elsif enable = '1' then
            -- Check if selection is valid
            if sel_index < NUM_INPUTS then
                output <= input_vectors(sel_index);
            else
                -- For invalid selection, default to zeros or first input
                output <= (others => '0');
            end if;
        end if;
    end process;
end Behavioral;
