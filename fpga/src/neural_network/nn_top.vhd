library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;
use ieee.numeric_std.all;

entity nn_top is
  generic (
    ADDRESS_WIDTH : integer := 8; -- Can be adjusted as needed
    FITNESS_WIDTH : integer := 16 -- Range 0-31 bits as specified
  );
  port (
    -- Input ports
    clk            : in std_logic;                                         -- System clock
    rst            : in std_logic;                                         -- Reset signal
    state          : in std_logic;                                         -- 1-bit state signal
    game_state     : in std_logic_vector(9 downto 0);                      -- 10-bit game state
    weights        : in std_logic_vector(ADDRESS_WIDTH * 32 - 1 downto 0); -- Weights for the neural network
    en_read_weight : in std_logic;                                         -- Enable signal from external decoder
    address        : in std_logic_vector(ADDRESS_WIDTH - 1 downto 0);      -- Address with one hot encoding

    -- Output ports
    next_action : out std_logic_vector(FITNESS_WIDTH - 1 downto 0) -- Fitness output (0-31 bits)
  );
end entity nn_top;

architecture rtl of nn_top is

  -- Internal signals
  -- TODO: set up the weights for the nural net once the othe components are tested
  signal internal_state       : std_logic;
  signal processed_game_state : std_logic_vector(9 downto 0);
  signal decoded_address      : integer range 0 to ADDRESS_WIDTH - 1;

begin

  -- Process to convert one-hot encoding to integer
  convert_to_integer : process (address) is
  begin
    decoded_address <= 0; -- Default value
    for i in 0 to ADDRESS_WIDTH - 1 loop
      if address(i) = '1' then
        decoded_address <= i;
      end if;
    end loop;
  end process;

  -- Main process for neural network computation
  nn_process : process (clk) is
  begin
    if rising_edge(clk) then
      if rst = '1' then
        next_action          <= (others => '0');
        internal_state       <= '0';
        processed_game_state <= (others => '0');
      else
        -- Store the inputs
        internal_state       <= state;
        processed_game_state <= game_state;

        -- Only process when enabled by the external decoder
        if en_read_weight = '1' then
          -- Example computation (replace with actual neural network logic)
          -- This is just a placeholder that combines the inputs to generate a fitness value
          if state = '1' then
            next_action <= std_logic_vector(resize(unsigned(game_state) + decoded_address, FITNESS_WIDTH));
          else
            next_action <= std_logic_vector(resize(unsigned(game_state), FITNESS_WIDTH));
          end if;
        end if;
      end if;
    end if;
  end process;

end architecture rtl;


