library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.fixed_pkg.all;
use ieee.fixed_float_types.all;
use ieee.math_real.log2;
use ieee.math_real.ceil;

package game_types is

  -- constants
  constant MAP_TILES_BITS       : integer := 8;
  constant MAP_MAX_SIZE_TILES   : integer := 2 ** MAP_TILES_BITS;
  constant TILE_PX_BITS         : integer := 3;
  constant TILE_PX              : integer := 2 ** TILE_PX_BITS;
  constant MAP_MAX_SIZE_PX_BITS : integer := MAP_TILES_BITS + TILE_PX_BITS;
  constant MAP_MAX_SIZE_PX      : integer := 2 ** MAP_MAX_SIZE_PX_BITS;

  constant F4_UPPER : integer := 11;
  constant F4_LOWER : integer := 4;

  -- main fixed point format used in game logic
  subtype f4_t is sfixed(F4_UPPER downto -F4_LOWER);

  type f4_vec_t is record
    x : f4_t;
    y : f4_t;
  end record f4_vec_t;
  function default_f4_vec_t return f4_vec_t;

  -- player type describes the state for one player
  type player_t is record
    pos : f4_vec_t;
    vel : f4_vec_t;
    score        : signed(15 downto 0);
    dead_timeout : unsigned(7 downto 0);
  end record player_t;
  function default_player_t return player_t;

  type tilepos_t is record
    -- spans the max map size
    x : unsigned(MAP_TILES_BITS - 1 downto 0);
    y : unsigned(MAP_TILES_BITS - 1 downto 0);
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
    width  : unsigned(MAP_TILES_BITS downto 0);
    height : unsigned(MAP_TILES_BITS downto 0);
  end record tilemap_t;
  function default_tilemap_t return tilemap_t;

  subtype s_coord_t is signed(MAP_MAX_SIZE_PX_BITS downto 0);

  -- helper functions on tiles
  function tile_is_solid(tile : tile_t) return boolean;
  function tile_is_water(tile : tile_t) return boolean;
  function get_tile(m : tilemap_t; pos : f4_vec_t) return tile_t;
  function get_tile(m : tilemap_t; pixel_x : s_coord_t; pixel_y : s_coord_t) return tile_t;

end package game_types;

package body game_types is

  function default_f4_vec_t return f4_vec_t is
    variable val : f4_vec_t := (
      x => to_sfixed(0.0, F4_UPPER, -F4_LOWER),
      y => to_sfixed(0.0, F4_UPPER, -F4_LOWER)
    );
  begin
    return val;
  end function;

  -- function implementations
  -- defaults
  function default_player_t return player_t is
    variable val : player_t := (
      pos          => default_f4_vec_t,
      vel          => default_f4_vec_t,
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

  -- helper functions
  function tile_is_solid(tile : tile_t) return boolean is
  begin
    return tile = TILE_GROUND or tile = TILE_SPRING or tile = TILE_ICE;
  end function;

  function tile_is_water(tile : tile_t) return boolean is
  begin
    return tile = TILE_WATER_BODY or tile = TILE_WATER_TOP;
  end function;

  function get_tile(m : tilemap_t; pos : f4_vec_t) return tile_t is
    variable pixel_x        : integer;
    variable pixel_y        : integer;
    variable tile_x         : integer;
    variable tile_y         : integer;
    variable signed_pixel_x : signed(31 downto 0);  -- using 32 bits to safely handle shifts (probably not necessary)
    variable signed_pixel_y : signed(31 downto 0);
  begin
    -- convert fixed-point coordinates to integers (pixels)
    -- use truncate (no rounding) and wrap behavior, no need for rounding here
    pixel_x := to_integer(pos.x, fixed_wrap, fixed_truncate);
    pixel_y := to_integer(pos.y, fixed_wrap, fixed_truncate);

    -- convert pixel coordinates to signed values for shift operation
    signed_pixel_x := to_signed(pixel_x, 32);
    signed_pixel_y := to_signed(pixel_y, 32);

    -- convert from pixels to tiles by right-shifting
    -- this is equivalent to division by TILE_PX (which is 2^TILE_PX_BITS)
    tile_x := to_integer(shift_right(signed_pixel_x, TILE_PX_BITS));
    tile_y := to_integer(shift_right(signed_pixel_y, TILE_PX_BITS));

    -- apply bounds checking
    if (tile_x < 0) or (tile_y < 0) or (tile_x >= to_integer(m.width)) then
      return TILE_GROUND;
    end if;

    if tile_y >= to_integer(m.height) then
      return TILE_AIR;
    end if;

    -- access the map, flipping y-coordinate for y-up ordering
    return m.m(to_integer(m.height) - 1 - tile_y, tile_x);
  end function;

  function get_tile(m : tilemap_t; pixel_x : s_coord_t; pixel_y : s_coord_t) return tile_t is
    variable tile_x : integer;
    variable tile_y : integer;
  begin
    -- convert pixel coordinates to signed values for shift operation
    -- this is equivalent to division by TILE_PX (which is 2^TILE_PX_BITS)
    tile_x := to_integer(shift_right(pixel_x, TILE_PX_BITS));
    tile_y := to_integer(shift_right(pixel_y, TILE_PX_BITS));

    -- apply bounds checking
    if (tile_x < 0) or (tile_y < 0) or (tile_x >= to_integer(m.width)) then
      return TILE_GROUND;
    end if;

    if tile_y >= to_integer(m.height) then
      return TILE_AIR;
    end if;

    -- access the map, flipping y-coordinate for y-up ordering
    return m.m(to_integer(m.height) - 1 - tile_y, tile_x);
  end function;

end package body game_types;
