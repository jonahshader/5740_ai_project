library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.fixed_pkg.all;
use ieee.fixed_float_types.all;
use ieee.math_real.log2;
use ieee.math_real.ceil;

package game_types is

  -- constants
  constant MAP_TILES_BITS       : integer := 4;                             -- bits needed to describe tile position
  constant MAP_MAX_SIZE_TILES   : integer := 2 ** MAP_TILES_BITS;           -- map max size (width height) in tiles
  constant TILE_PX_BITS         : integer := 3;                             -- bits needed to describe PX position within tile
  constant TILE_PX              : integer := 2 ** TILE_PX_BITS;             -- tile size (width height) in pixels
  constant MAP_MAX_SIZE_PX_BITS : integer := MAP_TILES_BITS + TILE_PX_BITS; -- bits needed to describe pixel position within the full map
  constant MAP_MAX_SIZE_PX      : integer := 2 ** MAP_MAX_SIZE_PX_BITS;     -- map max size (width height) in pixels

  constant MAP_MAX_SPAWNS : integer := MAP_MAX_SIZE_TILES * MAP_MAX_SIZE_TILES / 2;

  constant PLAYER_WIDTH       : integer := TILE_PX - 2;
  constant PLAYER_HEIGHT      : integer := TILE_PX - 2;
  constant PLAYER_KILL_HEIGHT : integer := PLAYER_HEIGHT / 2; -- height at which player is killed

  constant POINTS_PER_COIN : integer := 3;
  constant POINTS_PER_KILL : integer := 1;
  constant DEAD_TIMEOUT    : integer := 60;

  -- upper and lower bits for fixed point format
  constant F4_UPPER : integer := 11;
  constant F4_LOWER : integer := 4;

  -- main fixed point format used in game logic
  subtype f4_t is sfixed(F4_UPPER downto -F4_LOWER);

  -- helper function to simplify the conversion from raw integer to fixed point
  function from_raw(value : integer) return f4_t;

  -- gravity and acceleration constants
  constant JUMP_VEL          : f4_t := from_raw(25);
  constant JUMP_MIDAIR_ACCEL : f4_t := from_raw(1);
  constant SPRING_VEL        : f4_t := from_raw(38);
  constant MOVE_ACCEL        : f4_t := from_raw(3);
  -- constant MOVE_ACCEL : f4_t := "0000000000000000";
  constant MOVE_ACCEL_WATER : f4_t := from_raw(2); -- slower in water
  constant MOVE_ACCEL_ICE   : f4_t := from_raw(1); -- much slower on ice
  constant MOVE_MAX_VEL     : f4_t := from_raw(10);
  constant GRAVITY          : f4_t := from_raw(-2);
  constant GRAVITY_WATER    : f4_t := from_raw(-1);
  constant FALL_MAX_VEL     : f4_t := from_raw(-20);

  type f4_vec_t is record
    x : f4_t;
    y : f4_t;
  end record f4_vec_t;
  function default_f4_vec_t return f4_vec_t;

  -- player type describes the state for one player
  type player_t is record
    pos          : f4_vec_t;
    vel          : f4_vec_t;
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

  subtype tile_t is std_logic_vector(2 downto 0);

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
  type spawn_t is array (0 to MAP_MAX_SPAWNS - 1) of tilepos_t;
  function default_spawn_t return spawn_t;

  -- a tilemap contains a map_t, spawn_t, width and height
  type tilemap_t is record
    m              : map_t;
    spawn          : spawn_t;
    num_spawn      : unsigned(7 downto 0);
    num_spawn_bits : unsigned(3 downto 0);
    width          : unsigned(MAP_TILES_BITS downto 0); -- in tiles
    height         : unsigned(MAP_TILES_BITS downto 0); -- in tiles
  end record tilemap_t;
  function default_tilemap_t return tilemap_t;

  subtype pixelcoord_t is signed(MAP_MAX_SIZE_PX_BITS downto 0);

  -- helper functions on tiles
  function is_solid(tile : tile_t) return boolean;
  function is_water(tile : tile_t) return boolean;
  function get_tile(m : tilemap_t; pos : f4_vec_t) return tile_t;
  function get_tile(m : tilemap_t; pixel_x : pixelcoord_t; pixel_y : pixelcoord_t) return tile_t;
  function get_tile(m : tilemap_t; tile_x : integer; tile_y : integer) return tile_t;
  function get_tile_id(pixels : integer) return integer;
  function set_tile(m : tilemap_t; tile_pos : tilepos_t; tile : tile_t) return tilemap_t;
  function tile_to_pixel(tile : integer) return integer;
  function integer_to_f4(pixels : integer) return f4_t;
  -- function f4_to_pixelcoord(p : f4_t) return pixelcoord_t;

  function sample_spawn(m : tilemap_t; rng : std_logic_vector(31 downto 0)) return tilepos_t;
  function to_f4_vec(pos : tilepos_t) return f4_vec_t;

end package game_types;

package body game_types is

  function from_raw(value : integer) return f4_t is
  begin
    return resize(scalb(to_sfixed(value, F4_UPPER, -F4_LOWER), -F4_LOWER), F4_UPPER, -F4_LOWER);
  end function;

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
    variable val : map_t;
  begin
    for i in 0 to MAP_MAX_SIZE_TILES - 1 loop
      for j in 0 to MAP_MAX_SIZE_TILES - 1 loop
        val(i, j) := TILE_NOTHING;
      end loop;
    end loop;
    return val;
  end function;

  function default_spawn_t return spawn_t is
    variable val : spawn_t := (others => default_tilepos_t);
  begin
    return val;
  end function;

  function default_tilemap_t return tilemap_t is
    variable val : tilemap_t := (
      m      => default_map_t,
      spawn  => default_spawn_t,
      num_spawn      => (others => '0'),
      num_spawn_bits => (others => '0'),
      width  => (others => '0'),
      height => (others => '0')
    );
  begin
    return val;
  end function;

  -- helper functions
  function is_solid(tile : tile_t) return boolean is
  begin
    return tile = TILE_GROUND or tile = TILE_SPRING or tile = TILE_ICE;
  end function;

  function is_water(tile : tile_t) return boolean is
  begin
    return tile = TILE_WATER_BODY or tile = TILE_WATER_TOP;
  end function;

  function get_tile(m : tilemap_t; pos : f4_vec_t) return tile_t is
    variable pixel_x : integer;
    variable pixel_y : integer;
    variable tile_x  : integer;
    variable tile_y  : integer;
  begin
    -- convert fixed-point coordinates to integers (pixels)
    -- use truncate (no rounding) and wrap behavior, no need for rounding here
    pixel_x := to_integer(pos.x, fixed_wrap, fixed_truncate);
    pixel_y := to_integer(pos.y, fixed_wrap, fixed_truncate);

    -- convert from pixels to tiles by right-shifting
    -- this is equivalent to division by TILE_PX (which is 2^TILE_PX_BITS)
    tile_x := to_integer(shift_right(to_signed(pixel_x, 32), TILE_PX_BITS));
    tile_y := to_integer(shift_right(to_signed(pixel_y, 32), TILE_PX_BITS));

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

  function get_tile(m : tilemap_t; pixel_x : pixelcoord_t; pixel_y : pixelcoord_t) return tile_t is
    variable tile_x : integer;
    variable tile_y : integer;
  begin
    -- convert pixel coordinates to signed values for shift operation
    -- this is equivalent to division by TILE_PX (which is 2^TILE_PX_BITS)
    tile_x := to_integer(shift_right(pixel_x, TILE_PX_BITS));
    tile_y := to_integer(shift_right(pixel_y, TILE_PX_BITS));

    -- apply bounds checking
    if tile_x < 0 or tile_y < 0 or tile_x >= to_integer(m.width) then
      return TILE_GROUND;
    end if;

    if tile_y >= to_integer(m.height) then
      return TILE_AIR;
    end if;

    -- access the map, flipping y-coordinate for y-up ordering
    return m.m(to_integer(m.height) - 1 - tile_y, tile_x);
  end function;

  function get_tile(m : tilemap_t; tile_x : integer; tile_y : integer) return tile_t is
  begin
    if tile_x < 0 or tile_y < 0 or tile_x >= to_integer(m.width) then
      return TILE_GROUND;
    end if;

    if tile_y >= to_integer(m.height) then
      return TILE_AIR;
    end if;

    -- access the map, flipping y-coordinate for y-up ordering
    return m.m(to_integer(m.height) - 1 - tile_y, tile_x);
  end function;

  function get_tile_id(pixels : integer) return integer is
  begin
    return to_integer(shift_right(to_signed(pixels, 32), TILE_PX_BITS));
  end function;

  function set_tile(m : tilemap_t; tile_pos : tilepos_t; tile : tile_t) return tilemap_t is
    variable nm : tilemap_t;
  begin
    nm                                                                              := m;
    nm.m(to_integer(m.height) - 1 - to_integer(tile_pos.y), to_integer(tile_pos.x)) := tile;
    return nm;
  end function;

  function tile_to_pixel(tile : integer) return integer is
  begin
    return to_integer(shift_left(to_signed(tile, MAP_MAX_SIZE_PX_BITS + 1), TILE_PX_BITS));
  end function;

  function integer_to_f4(pixels : integer) return f4_t is
  begin
    return to_sfixed(pixels, F4_UPPER, -F4_LOWER);
  end function;

  function sample_spawn(m : tilemap_t; rng : std_logic_vector(31 downto 0)) return tilepos_t is
    variable index : integer;
  begin
    -- grab num_spawn_bits number of bits form rng
    -- index := shift_right(unsigned(rng), to_integer(32 - m.num_spawn_bits));
    index := to_integer(resize(unsigned(rng), m.num_spawn_bits));

    -- reduce it if its over the max
    if index >= m.num_spawn then
      index := index - to_integer(m.num_spawn);
    end if;

    -- return the spawn tile
    return m.spawn(index);
  end function;

  function to_f4_vec(pos : tilepos_t) return f4_vec_t is
    variable vec     : f4_vec_t;
    variable pixel_x : unsigned(MAP_MAX_SIZE_PX_BITS - 1 downto 0);
    variable pixel_y : unsigned(MAP_MAX_SIZE_PX_BITS - 1 downto 0);
  begin
    -- resize pos.x and pos.y to match the target size before shifting
    pixel_x := resize(shift_left(resize(pos.x, MAP_MAX_SIZE_PX_BITS), TILE_PX_BITS), MAP_MAX_SIZE_PX_BITS);
    pixel_y := resize(shift_left(resize(pos.y, MAP_MAX_SIZE_PX_BITS), TILE_PX_BITS), MAP_MAX_SIZE_PX_BITS);

    -- convert from unsigned to sfixed
    vec.x := to_sfixed(to_integer(pixel_x), F4_UPPER, -F4_LOWER);
    vec.y := to_sfixed(to_integer(pixel_y), F4_UPPER, -F4_LOWER);
    return vec;
  end function;

end package body game_types;
