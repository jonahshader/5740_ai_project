library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity NN_tb is
end NN_tb;

architecture Behavioral of NN_tb is

    -- Parameters for the neural network
    constant NUM_INPUTS : integer := 8;
    constant NUM_HIDDEN_LAYERS : integer := 2;
    constant NUM_HIDDEN_NEURONS : integer := 4;
    constant NUM_OUTPUTS : integer := 2;
    constant DATA_WIDTH : integer := 32;

    -- Signals for inputs, outputs, and internal connections
    signal clk : std_logic := '0';
    signal rst : std_logic := '1';
    signal inputs : std_logic_vector(NUM_INPUTS * DATA_WIDTH - 1 downto 0);
    signal weights_hidden : std_logic_vector(NUM_HIDDEN_LAYERS * NUM_HIDDEN_NEURONS * NUM_INPUTS * DATA_WIDTH - 1 downto 0);
    signal weights_output : std_logic_vector(NUM_OUTPUTS * NUM_HIDDEN_NEURONS * DATA_WIDTH - 1 downto 0);
    signal bias_hidden : std_logic_vector(NUM_HIDDEN_LAYERS * NUM_HIDDEN_NEURONS * DATA_WIDTH - 1 downto 0);
    signal bias_output : std_logic_vector(NUM_OUTPUTS * DATA_WIDTH - 1 downto 0);
    signal hidden_outputs : std_logic_vector(NUM_HIDDEN_LAYERS * NUM_HIDDEN_NEURONS * DATA_WIDTH - 1 downto 0);
    signal output : std_logic_vector(NUM_OUTPUTS * DATA_WIDTH - 1 downto 0);
    signal derivative : std_logic;

begin

    -- Clock generation
    clk_process : process
    begin
        while true loop
            clk <= '0';
            wait for 10 ns;
            clk <= '1';
            wait for 10 ns;
        end loop;
    end process;

    -- Instantiate the top-level neural network
    NN_inst : entity work.NN
        generic map (
            NUM_INPUTS => NUM_INPUTS,
            NUM_HIDDEN_LAYERS => NUM_HIDDEN_LAYERS,
            NUM_HIDDEN_NEURONS => NUM_HIDDEN_NEURONS,
            NUM_OUTPUTS => NUM_OUTPUTS,
            DATA_WIDTH => DATA_WIDTH
        )
        port map (
            clk => clk,
            rst => rst,
            inputs => inputs,
            weights_hidden => weights_hidden,
            weights_output => weights_output,
            bias_hidden => bias_hidden,
            bias_output => bias_output,
            hidden_outputs => hidden_outputs,
            output => output,
            derivative => derivative
        );

    -- Testbench stimulus process
    stimulus_process : process
    begin
        -- Reset the neural network
        rst <= '1';
        wait for 20 ns;
        rst <= '0';
        wait for 20 ns;

        -- Apply test inputs
        inputs <= (others => '0'); -- Example input
        wait for 20 ns;

        -- Add more test cases as needed
        -- ...

        wait;
    end process;

end Behavioral;