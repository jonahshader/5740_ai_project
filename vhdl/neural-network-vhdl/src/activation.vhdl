library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_SIGNED.ALL;
use work.components.all;

-- Create package work.compenents.all;

entity ReLu is
    Port (sum: in std_logic_vector(31 downto 0);
          a: out std_logic_vector(31 downto 0);
          a_Prime: out std_logic
          );
end ReLu;

architecture Behavioral of ReLu is

signal comp_Out: std_logic;
signal zeros: std_logic_vector(31 downto 0):= X"00000000";

begin

Mux1: Mux2to1 generic map(N => 32) port map(a => zeros, b => sum, s => comp_Out, y => a);

process(zeros,sum)
begin
    if(sum > zeros) then
        comp_Out <= '1';
    else
        comp_Out <= '0';
    end if;
end process;

a_Prime <= comp_Out;

end Behavioral;