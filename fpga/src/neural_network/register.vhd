library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

-- N-bit Register
-- E = '1', sclr = '0' --> Input data 'D' is copied on Q
-- E = '1', sclr = '1' --> Q is cleared (0)
entity reg is
   generic (N: INTEGER:= 4);
	port ( clock        : in std_logic;
            resetn      : in std_logic;
            E           : in std_logic; -- E: Enable signal
            sclr        : in std_logic; -- sclr: Synchronous clear
            D           : in std_logic_vector (N-1 downto 0);
            Q           : out std_logic_vector (N-1 downto 0));
end reg;

architecture Behavioral of reg is

	signal Qt: std_logic_vector (N-1 downto 0);
	
begin

	process (resetn, clock,E,sclr)
	begin
		if resetn = '0' then
			Qt <= (others => '0');
		elsif (clock'event and clock = '1') then
			if E = '1' then	
				if sclr = '1' then
					Qt <= (others => '0');
				else
					Qt <= D;
				end if;
			end if;
		end if;
		
	end process;

	Q <= Qt;
end Behavioral;
