library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.fixed_pkg.all;
use ieee.math_real.log2;
use ieee.math_real.ceil;

package game_types is

  -- constants
  constant MAP_SIZE_BITS      : integer := 8;
  constant MAP_MAX_SIZE_TILES : integer := 2 ** MAP_SIZE_BITS;

  constant F4_UPPER : integer := 11;
  constant F4_LOWER : integer := 4;

  -- main fixed point format used in game logic
  subtype f4_t is sfixed(F4_UPPER downto -F4_LOWER);

  -- player type describes the state for one player
  type player_t is record
    x            : f4_t;
    y            : f4_t;
    x_vel        : f4_t;
    y_vel        : f4_t;
    score        : signed(15 downto 0);
    dead_timeout : unsigned(7 downto 0);
  end record player_t;
  function default_player_t return player_t;

  type tilepos_t is record
    -- spans the max map size
    x : unsigned(MAP_SIZE_BITS - 1 downto 0);
    y : unsigned(MAP_SIZE_BITS - 1 downto 0);
  end record tilepos_t;
  function default_tilepos_t return tilepos_t;

  -- game state encompasses everything but the map
  -- note: game logic module includes rng. i suppose this info doesn't need to be
  -- transferred to PS, so its fine that it isn't included here.
  type gamestate_t is record
    p1       : player_t;
    p2       : player_t;
    coin_pos : tilepos_t;
    age      : unsigned(15 downto 0);
  end record gamestate_t;
  function default_gamestate_t return gamestate_t;

  type playerinput_t is record
    left  : boolean;
    right : boolean;
    jump  : boolean;
  end record playerinput_t;
  function default_playerinput_t return playerinput_t;

  -- map related stuff:
  -- TODO: I think this could be an enum type, but then the
  -- bit patterns are not explicitly known.

  subtype  tile_t is std_logic_vector(2 downto 0);
  constant TILE_NOTHING    : tile_t := "000";
  constant TILE_GROUND     : tile_t := "001";
  constant TILE_AIR        : tile_t := "010";
  constant TILE_SPRING     : tile_t := "011";
  constant TILE_WATER_BODY : tile_t := "100";
  constant TILE_WATER_TOP  : tile_t := "101";
  constant TILE_ICE        : tile_t := "110";
  constant TILE_COIN       : tile_t := "111";

  -- helper functions on tiles
  function tile_is_solid(tile : tile_t) return boolean;
  function tile_is_water(tile : tile_t) return boolean;

  -- 2d array to store a max-size map
  type map_t is array (0 to MAP_MAX_SIZE_TILES - 1, 0 to MAP_MAX_SIZE_TILES - 1) of tile_t;
  function default_map_t return map_t;

  -- 1d array for spawn tile spawn locations
  type spawn_t is array (0 to MAP_MAX_SIZE_TILES - 1) of tile_t;
  function default_spawn_t return spawn_t;

  -- a tilemap contains a map_t, spawn_t, width and height
  type tilemap_t is record
    m      : map_t;
    spawn  : spawn_t;
    width  : unsigned(MAP_SIZE_BITS - 1 downto 0);
    height : unsigned(MAP_SIZE_BITS - 1 downto 0);
  end record tilemap_t;
  function default_tilemap_t return tilemap_t;

end package game_types;

package body game_types is

  -- function implementations
  -- defaults
  function default_player_t return player_t is
    variable val : player_t := (
      x            => to_sfixed(0.0, F4_UPPER, -F4_LOWER),
      y            => to_sfixed(0.0, F4_UPPER, -F4_LOWER),
      x_vel        => to_sfixed(0.0, F4_UPPER, -F4_LOWER),
      y_vel        => to_sfixed(0.0, F4_UPPER, -F4_LOWER),
      score        => (others => '0'),
      dead_timeout => (others => '0')
    );
  begin
    return val;
  end function;

  function default_tilepos_t return tilepos_t is
    variable val : tilepos_t := (
      x => (others => '0'),
      y => (others => '0')
    );
  begin
    return val;
  end function;

  function default_gamestate_t return gamestate_t is
    variable val : gamestate_t := (
      p1 => default_player_t,
      p2 => default_player_t,
      coin_pos => default_tilepos_t,
      age => (others => '0')
    );
  begin
    return val;
  end function;

  function default_playerinput_t return playerinput_t is
    variable val : playerinput_t := (
      left => false,
      right => false,
      jump => false
    );
  begin
    return val;
  end function;

  function tile_is_solid(tile : tile_t) return boolean is
  begin
    return tile = TILE_GROUND or tile = TILE_SPRING or tile = TILE_ICE;
  end function;

  function tile_is_water(tile : tile_t) return boolean is
  begin
    return tile = TILE_WATER_BODY or tile = TILE_WATER_TOP;
  end function;

  function default_map_t return map_t is
    variable val : map_t := (
      (others => (others => TILE_NOTHING))
    );
  begin
    return val;
  end function;

  function default_spawn_t return spawn_t is
    variable val : spawn_t := (
      (others => TILE_NOTHING)
    );
  begin
    return val;
  end function;

  function default_tilemap_t return tilemap_t is
    variable val : tilemap_t := (
      m      => default_map_t,
      spawn  => default_spawn_t,
      width  => (others => '0'),
      height => (others => '0')
    );
  begin
    return val;
  end function;

end package body game_types;
