library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_SIGNED.ALL;
use IEEE.NUMERIC_STD.ALL;
use work.components.all;

entity Generic_Neuron is
    generic (
        NUM_INPUTS : integer := 8;          -- Configurable number of inputs
        DATA_WIDTH : integer := 32          -- Configurable data width
    );
    Port (
        clk       : in std_logic;           -- Clock signal
        rst       : in std_logic;           -- Reset signal
        weights   : in std_logic_vector(NUM_INPUTS*DATA_WIDTH-1 downto 0);  -- Packed weights
        inputs    : in std_logic_vector(NUM_INPUTS*DATA_WIDTH-1 downto 0);  -- Packed inputs
        bias      : in std_logic_vector(DATA_WIDTH-1 downto 0);             -- Bias term
        output    : out std_logic_vector(DATA_WIDTH-1 downto 0);            -- Neuron output
        derivative: out std_logic                                           -- For backpropagation
    );
end Generic_Neuron;

architecture Behavioral of Generic_Neuron is
    -- Internal signals
    type input_array_type is array (0 to NUM_INPUTS-1) of std_logic_vector(DATA_WIDTH-1 downto 0);
    signal weight_array : input_array_type;
    signal input_array : input_array_type;
    signal product_array : input_array_type;
    
    signal sum_result : std_logic_vector(DATA_WIDTH-1 downto 0);
    signal mult_temp : std_logic_vector(2*DATA_WIDTH-1 downto 0);
    signal accum_temp : std_logic_vector(2*DATA_WIDTH-1 downto 0);
    
begin
    -- Process to unpack the inputs and weights
    unpack_process: process(weights, inputs)
    begin
        for i in 0 to NUM_INPUTS-1 loop
            weight_array(i) <= weights((i+1)*DATA_WIDTH-1 downto i*DATA_WIDTH);
            input_array(i) <= inputs((i+1)*DATA_WIDTH-1 downto i*DATA_WIDTH);
        end loop;
    end process;
    
    -- Process to compute the weighted sum
    compute_process: process(clk, rst)
        variable acc : std_logic_vector(2*DATA_WIDTH-1 downto 0);
        variable prod : std_logic_vector(2*DATA_WIDTH-1 downto 0);
    begin
        if rst = '1' then
            accum_temp <= (others => '0');
        elsif rising_edge(clk) then
            -- Initialize accumulator
            acc := (others => '0');
            
            -- Multiply and accumulate
            for i in 0 to NUM_INPUTS-1 loop
                prod := weight_array(i) * input_array(i);
                acc := acc + prod;
            end loop;
            
            -- Add bias
            acc := acc + bias;
            
            -- Store result
            accum_temp <= acc;
        end if;
    end process;
    
    -- Extract proper bits for the sum (similar to original implementation)
    sum_result <= accum_temp(DATA_WIDTH+23 downto 24);
    
    -- Activation function (ReLU)
    activation: ReLu port map(
        sum => sum_result, 
        a => output, 
        a_Prime => derivative
    );
    
end Behavioral;