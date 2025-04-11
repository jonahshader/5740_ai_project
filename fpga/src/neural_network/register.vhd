---------------------------------------------------------------------------
-- This VHDL file was developed by Daniel Llamocca (2013).  It may be
-- freely copied and/or distributed at no cost.  Any persons using this
-- file for any purpose do so at their own risk, and are responsible for
-- the results of such use.  Daniel Llamocca does not guarantee that
-- this file is complete, correct, or fit for any particular purpose.
-- NO WARRANTY OF ANY KIND IS EXPRESSED OR IMPLIED.  This notice must
-- accompany any copy of this file.
--------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;

-- N-bit Register
-- E = '1', sclr = '0' --> Input data 'D' is copied on Q
-- E = '1', sclr = '1' --> Q is cleared (0)
entity regiseter is
  generic (
    N : integer := 4
  );
  port (
    clock  : in std_logic;
    resetn : in std_logic;
    e      : in std_logic; -- sclr: Synchronous clear
    d      : in std_logic_vector(N - 1 downto 0);
    q      : out std_logic_vector(N - 1 downto 0)
  );
end entity regiseter;

architecture behavioral of regiseter is

  signal qt : std_logic_vector(N - 1 downto 0);

begin

  process (resetn, clock, e) is
  begin
    if resetn = '0' then
      qt <= (others => '0');
    elsif clock'event and clock = '1' then
      if e = '1' then
        qt <= (others => '0');
      end if;
    end if;
  end process;

  q <= qt;

end architecture behavioral;

