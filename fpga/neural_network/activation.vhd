library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;
use work.components.all;

-- Create package work.compenents.all;

entity relu is
  port (
    sum     : in std_logic_vector(31 downto 0);
    a       : out std_logic_vector(31 downto 0);
    a_prime : out std_logic
  );
end entity relu;

architecture behavioral of relu is

  signal comp_out : std_logic;
  signal zeros    : std_logic_vector(31 downto 0) := x"00000000";

begin

  mux1 : component mux2to1
    generic map (
      N => 32
    )
    port map (
      a => zeros,
      b => sum,
      s => comp_out,
      y => a
    );

  process (zeros, sum) is
  begin
    if sum > zeros then
      comp_out <= '1';
    else
      comp_out <= '0';
    end if;
  end process;

  a_prime <= comp_out;

end architecture behavioral;
