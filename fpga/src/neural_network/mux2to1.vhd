library ieee;
use ieee.std_logic_1164.all;

entity mux2to1 is
  generic (
    N : integer
  );
  port (
    a : in std_logic_vector(N - 1 downto 0);
    b : in std_logic_vector(N - 1 downto 0);
    s : in std_logic;
    y : out std_logic_vector(N - 1 downto 0)
  );
end entity mux2to1;

architecture mux2to1 of mux2to1 is

begin

  process (a, b, s) is
  begin
    if s = '0' then
      y <= a;
    else
      y <= b;
    end if;
  end process;

end architecture mux2to1;
