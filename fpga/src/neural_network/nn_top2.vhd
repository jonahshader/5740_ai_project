library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;
use ieee.numeric_std.all;
use types.all; -- double check this syntax

-- number of weights = INPUTS + HIDDEN_LAYERS * NEURONS_PER_LAYER
-- weight width could be 4 to make it a nice number
--(inputs * connections + layer * neuron * connections)

entity nn_top is
    generic (
        ADDRESS_WIDTH : integer := 8;     -- Can be adjusted as needed
        FITNESS_WIDTH : integer := 16;     -- Range 0-31 bits as specified
        WEIGHT_WIDTH : integer := 3; 
        INPUTS : integer := 10; -- Number of inputs to the neural network
        NEURONS_PER_LAYER : integer := 8; -- Number of neurons in each layer
        HIDDEN_LAYERS : integer := 2; -- Number of hidden layers
        OUTPUTS : integer := 4 -- Number of outputs from the neural network
    );
    port (
        clk            : in std_logic;                               
        rst            : in std_logic;                                
        state          : in std_logic;               
        weight_in      : in std_logic_vector(WEIGHT_WIDTH - 1 downto 0);  
        wr_rd          : in std_logic;
        sclr_register  : in std_logic;   

        game_state     : in std_logic_vector(9 downto 0);             
        en_read_weight : in std_logic;                             
        address        : in std_logic_vector(ADDRESS_WIDTH-1 downto 0); 

        next_action : out std_logic_vector(FITNESS_WIDTH-1 downto 0) 
    );
end entity nn_top;

architecture structural of nn_top is

    component reg is
        generic (N: INTEGER:= 4);
         port (  clock        : in std_logic;
                 resetn      : in std_logic;
                 E           : in std_logic; 
                 sclr        : in std_logic;
                 D           : in std_logic_vector (N-1 downto 0);
                 Q           : out std_logic_vector (N-1 downto 0));
    end component;
     
    component neuron is
        generic (
          NUM_INPUTS : integer := 16; 
          DATA_WIDTH : integer := 32; 
          DATA_WEIGHT_WIDTH : integer := 3 
        );
      
        port (
          weights    : in weights(NUM_INPUTS - 1 downto 0, DATA_WEIGHT_WIDTH downto 0); 
          inputs     : in std_logic_vector(NUM_INPUTS - 1 downto 0, DATA_WIDTH - 1 downto 0); 
          bias       : in std_logic_vector(DATA_WIDTH - 1 downto 0);             
          output     : out std_logic_vector(DATA_WIDTH - 1 downto 0)           
        );
      end component;

    signal number_neurons: integer := INPUTS + HIDDEN_LAYERS * NEURONS_PER_LAYER;
    signal register_enbale: std_logic_vector(2**ADDRESS_WIDTH - 1 downto 0);
    signal weights_input_layer: weight_array(0 to INPUTS - 1, 0 to NEURONS_PER_LAYER - 1)(WEIGHT_WIDTH - 1 downto 0) := (others => (others => (others => '0')));
    
begin

    demux: decoder -- add enable
        generic map (
            INPUT_NUM => ADDRESS_WIDTH
        )
        port map (
            enable => wr_rd,
            sel => address,
            data_out => register_enbale
        );

    input_layer: process()
        begin

        --variable weights_input_layer: weight_array(0 to INPUTS - 1, 0 to NEURONS_PER_LAYER - 1)(WEIGHT_WIDTH - 1 downto 0) := (others => (others => (others => '0')));

            -- input layer
            for neurons in range 0 to INPUTS - 1 loop
                for connections in range 0 to NEURONS_PER_LAYER - 1 loop
                    weight_registers: register
                        generic map (
                            N => WEIGHT_WIDTH
                        )
                        port map (
                            clock => clk,
                            resetn => rst,
                            sclr => sclr_register,
                            e => register_enbale(neuron * connections),
                             
                            
                end loop;



    for i in range 0 to number_neurons loop
        -- Weight Registers
        weight_reg: register
            generic map (
                N => WEIGHT_WIDTH
            )
            port map (
                clock => clk,
                resetn => rst,
                sclr => sclr_register,
                e => register_enbale(i),
                d => weight_in,
                q => weights(i)
            );

        -- Neuron


    end loop;
                
























end architecture structural;