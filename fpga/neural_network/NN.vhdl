library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity NN is
    Generic (
        -- Data width parameters
        DATA_WIDTH          : integer := 16;     -- Width of data path
        WEIGHT_WIDTH        : integer := 16;     -- Width of weights
        
        -- Network architecture parameters
        NUM_INPUTS          : integer := 3;      -- Number of network inputs
        NUM_OUTPUTS         : integer := 2;      -- Number of network outputs
        NUM_HIDDEN_LAYERS   : integer := 2;      -- Number of hidden layers
        NEURONS_PER_LAYER   : integer := 4;      -- Neurons in each hidden layer
        
        -- Activation function selection (0: ReLU, 1: Sigmoid, 2: Tanh)
        ACTIVATION_TYPE     : integer := 0
    );
    Port (
        clk          : in  STD_LOGIC;                                  -- System clock
        rst          : in  STD_LOGIC;                                  -- Reset signal
        start        : in  STD_LOGIC;                                  -- Start processing signal
        data_in      : in  STD_LOGIC_VECTOR(NUM_INPUTS*DATA_WIDTH-1 downto 0); -- Input data
        data_valid   : in  STD_LOGIC;                                  -- Input data valid signal
        data_out     : out STD_LOGIC_VECTOR(NUM_OUTPUTS*DATA_WIDTH-1 downto 0); -- Output results
        data_ready   : out STD_LOGIC;                                  -- Output ready signal
        busy         : out STD_LOGIC                                   -- Network busy signal
    );
end NN;

architecture Behavioral of NN is

    -- Define types for network architecture
    type layer_data_type is array (natural range <>) of STD_LOGIC_VECTOR(DATA_WIDTH-1 downto 0);
    type weight_matrix_type is array (natural range <>, natural range <>) of STD_LOGIC_VECTOR(WEIGHT_WIDTH-1 downto 0);
    
    -- Component declaration for neuron
    component neuron
        generic (
            DATA_WIDTH     : integer;
            WEIGHT_WIDTH   : integer;
            NUM_INPUTS     : integer;
            ACTIVATION_TYPE: integer
        );
        port (
            clk            : in  STD_LOGIC;
            rst            : in  STD_LOGIC;
            enable         : in  STD_LOGIC;
            inputs         : in  STD_LOGIC_VECTOR(NUM_INPUTS*DATA_WIDTH-1 downto 0);
            weights        : in  STD_LOGIC_VECTOR(NUM_INPUTS*WEIGHT_WIDTH-1 downto 0);
            bias           : in  STD_LOGIC_VECTOR(WEIGHT_WIDTH-1 downto 0);
            output         : out STD_LOGIC_VECTOR(DATA_WIDTH-1 downto 0);
            done           : out STD_LOGIC
        );
    end component;
    
    -- State machine states
    type state_type is (IDLE, LOAD_INPUTS, PROCESS_LAYER, WAIT_LAYER, OUTPUT_RESULTS);
    signal state : state_type := IDLE;
    
    -- Layer data storage
    type network_layer_data_type is array (0 to NUM_HIDDEN_LAYERS) of layer_data_type(0 to NEURONS_PER_LAYER-1);
    signal layer_outputs : network_layer_data_type;
    signal input_data    : layer_data_type(0 to NUM_INPUTS-1);
    signal output_data   : layer_data_type(0 to NUM_OUTPUTS-1);
    
    -- Weight storage (would typically be initialized from memory or registers)
    -- For input to first hidden layer
    signal input_weights  : weight_matrix_type(0 to NEURONS_PER_LAYER-1, 0 to NUM_INPUTS-1);
    signal input_biases   : layer_data_type(0 to NEURONS_PER_LAYER-1);
    
    -- For hidden layers
    type hidden_weights_type is array (0 to NUM_HIDDEN_LAYERS-2) of 
        weight_matrix_type(0 to NEURONS_PER_LAYER-1, 0 to NEURONS_PER_LAYER-1);
    signal hidden_weights : hidden_weights_type;
    type hidden_biases_type is array (0 to NUM_HIDDEN_LAYERS-2) of 
        layer_data_type(0 to NEURONS_PER_LAYER-1);
    signal hidden_biases  : hidden_biases_type;
    
    -- For output layer
    signal output_weights : weight_matrix_type(0 to NUM_OUTPUTS-1, 0 to NEURONS_PER_LAYER-1);
    signal output_biases  : layer_data_type(0 to NUM_OUTPUTS-1);
    
    -- Control signals
    signal current_layer  : integer range 0 to NUM_HIDDEN_LAYERS := 0;
    signal layer_done     : STD_LOGIC_VECTOR(NEURONS_PER_LAYER-1 downto 0);
    signal output_done    : STD_LOGIC_VECTOR(NUM_OUTPUTS-1 downto 0);
    signal process_enable : STD_LOGIC := '0';
    signal output_enable  : STD_LOGIC := '0';
    
    -- Helper functions
    function flatten_weights(weights: weight_matrix_type; neuron_idx: integer; num_inputs: integer) return STD_LOGIC_VECTOR is
        variable flattened : STD_LOGIC_VECTOR(num_inputs*WEIGHT_WIDTH-1 downto 0);
    begin
        for i in 0 to num_inputs-1 loop
            flattened(WEIGHT_WIDTH*(i+1)-1 downto WEIGHT_WIDTH*i) := weights(neuron_idx, i);
        end loop;
        return flattened;
    end function;
    
    function flatten_inputs(inputs: layer_data_type) return STD_LOGIC_VECTOR is
        variable flattened : STD_LOGIC_VECTOR(inputs'length*DATA_WIDTH-1 downto 0);
    begin
        for i in 0 to inputs'length-1 loop
            flattened(DATA_WIDTH*(i+1)-1 downto DATA_WIDTH*i) := inputs(i);
        end loop;
        return flattened;
    end function;

begin

    -- Process to parse input data into separate neurons
    process(data_in)
    begin
        for i in 0 to NUM_INPUTS-1 loop
            input_data(i) <= data_in(DATA_WIDTH*(i+1)-1 downto DATA_WIDTH*i);
        end loop;
    end process;
    
    -- Process to combine output neurons into output data
    process(output_data)
    begin
        for i in 0 to NUM_OUTPUTS-1 loop
            data_out(DATA_WIDTH*(i+1)-1 downto DATA_WIDTH*i) <= output_data(i);
        end loop;
    end process;

    -- Main state machine for network processing
    process(clk, rst)
        variable all_done : boolean;
    begin
        if rst = '1' then
            state <= IDLE;
            current_layer <= 0;
            process_enable <= '0';
            output_enable <= '0';
            busy <= '0';
            data_ready <= '0';
        elsif rising_edge(clk) then
            case state is
                when IDLE =>
                    data_ready <= '0';
                    if start = '1' and data_valid = '1' then
                        busy <= '1';
                        current_layer <= 0;
                        state <= LOAD_INPUTS;
                    else
                        busy <= '0';
                    end if;
                    
                when LOAD_INPUTS =>
                    -- Start processing the first layer
                    process_enable <= '1';
                    state <= PROCESS_LAYER;
                    
                when PROCESS_LAYER =>
                    process_enable <= '0';  -- Only pulse the enable signal
                    state <= WAIT_LAYER;
                    
                when WAIT_LAYER =>
                    -- Check if all neurons in the current layer have finished
                    all_done := true;
                    if current_layer < NUM_HIDDEN_LAYERS then
                        -- Check hidden layer neurons
                        for i in 0 to NEURONS_PER_LAYER-1 loop
                            if layer_done(i) = '0' then
                                all_done := false;
                                exit;
                            end if;
                        end loop;
                    else
                        -- Check output layer neurons
                        for i in 0 to NUM_OUTPUTS-1 loop
                            if output_done(i) = '0' then
                                all_done := false;
                                exit;
                            end if;
                        end loop;
                    end if;
                    
                    -- Move to next layer or finish if all done
                    if all_done then
                        if current_layer < NUM_HIDDEN_LAYERS then
                            current_layer <= current_layer + 1;
                            
                            -- If this was the last hidden layer, enable the output layer
                            if current_layer = NUM_HIDDEN_LAYERS-1 then
                                output_enable <= '1';
                            else
                                process_enable <= '1';
                            end if;
                            
                            state <= PROCESS_LAYER;
                        else
                            -- We've processed all layers including the output
                            state <= OUTPUT_RESULTS;
                        end if;
                    end if;
                    
                when OUTPUT_RESULTS =>
                    data_ready <= '1';
                    busy <= '0';
                    output_enable <= '0';
                    
                    if start = '0' then  -- Wait until start is deasserted before accepting new inputs
                        state <= IDLE;
                    end if;
            end case;
        end if;
    end process;
    
    -- Generate the first hidden layer neurons
    FIRST_HIDDEN_GEN: for i in 0 to NEURONS_PER_LAYER-1 generate
        first_neuron: neuron
        generic map (
            DATA_WIDTH      => DATA_WIDTH,
            WEIGHT_WIDTH    => WEIGHT_WIDTH,
            NUM_INPUTS      => NUM_INPUTS,
            ACTIVATION_TYPE => ACTIVATION_TYPE
        )
        port map (
            clk             => clk,
            rst             => rst,
            enable          => process_enable,
            inputs          => flatten_inputs(input_data),
            weights         => flatten_weights(input_weights, i, NUM_INPUTS),
            bias            => input_biases(i),
            output          => layer_outputs(0)(i),
            done            => layer_done(i)
        );
    end generate;
    
    -- Generate the remaining hidden layers
    HIDDEN_LAYERS_GEN: for layer in 1 to NUM_HIDDEN_LAYERS-1 generate
        HIDDEN_NEURONS_GEN: for i in 0 to NEURONS_PER_LAYER-1 generate
            hidden_neuron: neuron
            generic map (
                DATA_WIDTH      => DATA_WIDTH,
                WEIGHT_WIDTH    => WEIGHT_WIDTH,
                NUM_INPUTS      => NEURONS_PER_LAYER,
                ACTIVATION_TYPE => ACTIVATION_TYPE
            )
            port map (
                clk             => clk,
                rst             => rst,
                enable          => process_enable and (current_layer = layer),
                inputs          => flatten_inputs(layer_outputs(layer-1)),
                weights         => flatten_weights(hidden_weights(layer-1), i, NEURONS_PER_LAYER),
                bias            => hidden_biases(layer-1)(i),
                output          => layer_outputs(layer)(i),
                done            => layer_done(i)
            );
        end generate;
    end generate;
    
    -- Generate the output layer neurons
    OUTPUT_LAYER_GEN: for i in 0 to NUM_OUTPUTS-1 generate
        output_neuron: neuron
        generic map (
            DATA_WIDTH      => DATA_WIDTH,
            WEIGHT_WIDTH    => WEIGHT_WIDTH,
            NUM_INPUTS      => NEURONS_PER_LAYER,
            ACTIVATION_TYPE => ACTIVATION_TYPE
        )
        port map (
            clk             => clk,
            rst             => rst,
            enable          => output_enable,
            inputs          => flatten_inputs(layer_outputs(NUM_HIDDEN_LAYERS-1)),
            weights         => flatten_weights(output_weights, i, NEURONS_PER_LAYER),
            bias            => output_biases(i),
            output          => output_data(i),
            done            => output_done(i)
        );
    end generate;

end Behavioral;