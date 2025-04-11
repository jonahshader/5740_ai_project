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

  function init_map return tilemap_t is
    variable m : tilemap_t := default_tilemap_t;
  begin
    m.width  := to_unsigned(8, m.width'length);
    m.height := to_unsigned(8, m.height'length);

    -- loop through and set them all to TILE_AIR
    for x in 0 to 7 loop
      for y in 0 to 7 loop
        m.m(y, x) := TILE_AIR;
      end loop;
    end loop;

    -- set the ground to TILE_GROUND
    -- note: this is in y-down, so the ground is at y = 7
    for x in 0 to 7 loop
      m.m(7, x) := TILE_GROUND;
    end loop;

    -- make a floating platform
    for x in 3 to 5 loop
      m.m(4, x) := TILE_GROUND;
    end loop;

    -- extend with ice
    m.m(4, 6) := TILE_ICE;

    -- replace a ground tile with TILE_SPRING
    m.m(7, 0) := TILE_SPRING;

    -- replace the next ground tile with TILE_WATER_TOP
    m.m(7, 1) := TILE_WATER_TOP;

    -- set spawns, which are the 6 remaining ground tiles
    for i in 2 to 7 loop
      m.spawn(i - 2) :=
      (
        x => to_unsigned(i, m.spawn(0).x'length),
        y => to_unsigned(1, m.spawn(0).y'length)
      );
    end loop;
    -- set the number of spawns
    m.num_spawn := to_unsigned(6, m.num_spawn'length);
    -- set the bits required to store num_spawn
    -- TODO: really i should be doing something like ceil(log2(num_spawn))
    -- m.num_spawn_bits := to_unsigned(3, m.num_spawn_bits'length);
    -- dear god...
    m.num_spawn_bits := to_unsigned(integer(ceil(log2(real(to_integer(m.num_spawn))))), m.num_spawn_bits'length);

    return m;
  end function;

  signal gamestate : gamestate_t;
-- signal m         : tilemap_t := init_map;

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
      m          => init_map,
      p1_input   => p1_input,
      p2_input   => p2_input,
      go         => go,
      done       => done,
      gamestate  => gamestate
    );

end architecture game_test_arch;
