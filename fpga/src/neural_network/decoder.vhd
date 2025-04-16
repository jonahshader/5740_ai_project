library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity decoder is
  generic (
    INPUT_NUM : natural := 4
  );
  port (
    enable   : in std_logic;
    sel      :    in std_logic_vector(INPUT_NUM - 1 downto 0);
    data_out :   out std_logic_vector(2 ** INPUT_NUM - 1 downto 0)
  );
end entity decoder;

architecture rtl of decoder is

begin

  demux : process (sel, enable) is
  begin
    data_out <= (others => '0');
    if enable = '1' then
      data_out(to_integer(unsigned(sel))) <= '1';
    end if;
  end process;

end architecture rtl;
