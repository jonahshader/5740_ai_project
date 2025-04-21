library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.fixed_pkg.all;
use ieee.fixed_float_types.all;
use ieee.math_real.log2;
use ieee.math_real.ceil;
use work.game_types.all;
use work.player_funs.all;
use work.custom_utils.all;

entity game_test is
  port (
    clk        : in std_logic;
    init       : in boolean;
    swap_start : in boolean;
    seed       : in std_logic_vector(31 downto 0);

    p1_input : in playerinput_t;
    p2_input : in playerinput_t;

    go   : in boolean;  -- start an update. only call when done is high
    done : out boolean; -- also goes high after init is done (high when in idle_s)

    p1_x     : out unsigned(F4_UPPER downto 0);
    p1_y     : out unsigned(F4_UPPER downto 0);
    p2_x     : out unsigned(F4_UPPER downto 0);
    p2_y     : out unsigned(F4_UPPER downto 0);
    p1_score : out signed(15 downto 0);
    p2_score : out signed(15 downto 0);
    age      : out unsigned(15 downto 0)
  );
end entity game_test;

architecture game_test_arch of game_test is

  signal gamestate : gamestate_t;

begin

  p1_x     <= to_unsigned(to_integer(gamestate.p1.pos.x, fixed_wrap, fixed_truncate), p1_x'length);
  p1_y     <= to_unsigned(to_integer(gamestate.p1.pos.y, fixed_wrap, fixed_truncate), p1_y'length);
  p2_x     <= to_unsigned(to_integer(gamestate.p2.pos.x, fixed_wrap, fixed_truncate), p2_x'length);
  p2_y     <= to_unsigned(to_integer(gamestate.p2.pos.y, fixed_wrap, fixed_truncate), p2_y'length);
  p1_score <= gamestate.p1.score;
  p2_score <= gamestate.p2.score;
  age      <= gamestate.age;

  -- instantiate
  g : entity work.game
    port map (
      clk        => clk,
      init       => init,
      swap_start => swap_start,
      seed       => seed,
      m          => test_tilemap_t,
      p1_input   => p1_input,
      p2_input   => p2_input,
      go         => go,
      done       => done,
      gamestate  => gamestate
    );

end architecture game_test_arch;
