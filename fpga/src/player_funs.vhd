library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.fixed_pkg.all;
use ieee.fixed_float_types.all;
use ieee.math_real.log2;
use ieee.math_real.ceil;
use work.game_types.all;

package player_funs is

  type player_setup_1_t is record
    x_tile_left     : integer;
    x_tile_right    : integer;
    y_tile_down     : integer;
    y_tile_up       : integer;
    left_tile       : tile_t;
    right_tile      : tile_t;
    down_left_tile  : tile_t;
    down_right_tile : tile_t;
    grounded        : boolean;
  end record player_setup_1_t;
  function default_player_setup_1_t return player_setup_1_t;

  type player_setup_2_t is record
    xn_tile_left  : integer;
    xn_tile_right : integer;
    yn_tile_down  : integer;
    yn_tile_up    : integer;
    left_1        : tile_t;
    left_2        : tile_t;
    right_1       : tile_t;
    right_2       : tile_t;
    down_1        : tile_t;
    down_2        : tile_t;
    up_1          : tile_t;
    up_2          : tile_t;
  end record player_setup_2_t;
  function default_player_setup_2_t return player_setup_2_t;

  function phase_1(p : player_t; other : player_t; input : playerinput_t; setup_1 : player_setup_1_t; m : tilemap_t) return player_t;
  function phase_2(p : player_t; p_spawn : tilepos_t; coin_pos : tilepos_t; setup_2 : player_setup_2_t; m : tilemap_t) return player_t;
  function is_touching_coin(p : player_t; coinpos : tilepos_t) return boolean;

  function phase_1_setup(p : player_t; m : tilemap_t) return player_setup_1_t;
  function phase_2_setup(p : player_t; m : tilemap_t; setup_1 : player_setup_1_t) return player_setup_2_t;

end package player_funs;

package body player_funs is

  -- Phase 1
  function phase_1(p : player_t; other : player_t; input : playerinput_t; setup_1 : player_setup_1_t; m : tilemap_t) return player_t is
    variable pn : player_t;

    variable in_water : boolean;
    variable on_ice   : boolean;

    variable grav     : f4_t := GRAVITY;
    variable move_acc : f4_t := MOVE_ACCEL;

    variable accel : boolean := true;
  begin
    -- start with current player state
    pn := p;

    -- early return if dead
    if pn.dead_timeout > 0 then
      return pn;
    end if;

    -- determine if in water
    in_water := is_water(setup_1.left_tile) or is_water(setup_1.right_tile);
    on_ice   := setup_1.down_left_tile = TILE_ICE or setup_1.down_right_tile = TILE_ICE;

    -- determine acceleration based on context
    grav     := GRAVITY_WATER when in_water else GRAVITY;
    move_acc := MOVE_ACCEL_ICE when on_ice else MOVE_ACCEL_WATER when in_water else MOVE_ACCEL;

    -- jump logic
    if setup_1.grounded then
      if setup_1.down_left_tile = TILE_SPRING or setup_1.down_right_tile = TILE_SPRING then
        -- spring tile
        pn.vel.y := SPRING_VEL;
      elsif input.jump then
        -- jump
        pn.vel.y := JUMP_VEL;
      end if;
    else -- not grounded. accelerate due to gravity
      pn.vel.y := resize(pn.vel.y + grav, pn.vel.y'high, pn.vel.y'low, fixed_wrap, fixed_truncate);
      if input.jump then
        -- if jump is held, also accelerate up a little to 'float'
        pn.vel.y := resize(pn.vel.y + JUMP_MIDAIR_ACCEL, pn.vel.y'high, pn.vel.y'low, fixed_wrap, fixed_truncate);
      end if;
      -- limit y_vel
      -- TODO: might save circuits to pull this out of the if statement
      if pn.vel.y < FALL_MAX_VEL then
        pn.vel.y := FALL_MAX_VEL;
      end if;
    end if;

    -- accelerate x_vel based on input
    if input.left and not input.right then
      -- accel left
      pn.vel.x := resize(pn.vel.x - move_acc, pn.vel.x'high, pn.vel.x'low, fixed_wrap, fixed_truncate);
    elsif input.right and not input.left then
      -- accel right
      pn.vel.x := resize(pn.vel.x + move_acc, pn.vel.x'high, pn.vel.x'low, fixed_wrap, fixed_truncate);
    else
      -- decelerate towards zero if grounded and not on ice
      if setup_1.grounded and not on_ice then
        if pn.vel.x > 0 then
          -- check if we have room to do the full speed reduction
          if pn.vel.x >= move_acc then
            pn.vel.x := resize(pn.vel.x - move_acc, pn.vel.x'high, pn.vel.x'low, fixed_wrap, fixed_truncate);
          else
            -- we are too slow to do the full speed reduction
            pn.vel.x := integer_to_f4(0);
          end if;
        elsif pn.vel.x < 0 then
          -- check if we have room to do the full speed reduction
          if pn.vel.x <= -move_acc then
            pn.vel.x := resize(pn.vel.x + move_acc, pn.vel.x'high, pn.vel.x'low, fixed_wrap, fixed_truncate);
          else
            -- we are too slow to do the full speed reduction
            pn.vel.x := integer_to_f4(0);
          end if;
        end if;
      end if;
    end if;

    -- limit x_vel
    if pn.vel.x < -MOVE_MAX_VEL then
      pn.vel.x := resize(-MOVE_MAX_VEL, pn.vel.x'high, pn.vel.x'low, fixed_wrap, fixed_truncate);
    end if;
    if pn.vel.x > MOVE_MAX_VEL then
      pn.vel.x := MOVE_MAX_VEL;
    end if;

    -- accelerate based on collision with other player
    if other.dead_timeout = 0 then -- only run when other guy is alive
      if abs(pn.pos.y - other.pos.y) < PLAYER_HEIGHT then -- if overlapping in y axis
        if abs(pn.pos.x - other.pos.x) <= PLAYER_WIDTH then -- if overlapping in x axis
          -- if other player is significantly above this one, die
          if other.pos.y >= p.pos.y + PLAYER_KILL_HEIGHT then
            pn.dead_timeout := to_unsigned(DEAD_TIMEOUT, pn.dead_timeout'length);
          -- if the opposite is true, gain a point
          elsif pn.pos.y >= other.pos.y + PLAYER_KILL_HEIGHT then
            pn.score := pn.score + POINTS_PER_KILL;
            accel    := false; -- don't accel from collision if it results in a kill
          end if;

          if accel then
            if pn.pos.x > other.pos.x then
              pn.vel.x := resize(pn.vel.x + other.pos.x - pn.pos.x + integer_to_f4(PLAYER_WIDTH), pn.vel.x'high, pn.vel.x'low, fixed_wrap, fixed_truncate);
            elsif pn.pos.x < other.pos.x then
              pn.vel.x := resize(pn.vel.x + other.pos.x - pn.pos.x - integer_to_f4(PLAYER_WIDTH), pn.vel.x'high, pn.vel.x'low, fixed_wrap, fixed_truncate);
            end if;
          end if;
        end if;
      end if;
    end if;

    -- integrate vel
    -- TODO: might need to go into its own cycle?
    pn.pos.x := resize(pn.pos.x + pn.vel.x, pn.pos.x'high, pn.pos.x'low, fixed_wrap, fixed_truncate);
    pn.pos.y := resize(pn.pos.y + pn.vel.y, pn.pos.y'high, pn.pos.y'low, fixed_wrap, fixed_truncate);

    return pn;
  end function;

  -- Phase 2
  function phase_2(p : player_t; p_spawn : tilepos_t; coin_pos : tilepos_t; setup_2 : player_setup_2_t; m : tilemap_t) return player_t is
    variable pn : player_t;
  begin
    -- start with current player state
    pn := p;

    -- early return if dead
    if pn.dead_timeout > 1 then
      -- decrement dead counter
      pn.dead_timeout := pn.dead_timeout - 1;
      return pn;
    elsif p.dead_timeout = 1 then
      pn.dead_timeout := to_unsigned(0, pn.dead_timeout'length);
      -- respawn
      pn.pos := to_f4_vec(p_spawn);
      pn.vel := default_f4_vec_t;
      -- early return. (this is different than c++ version. i suspect this reduces combinatorial complexity)
      return pn;
    end if;

    -- handle left right collisions
    if pn.vel.x < 0 then
      -- going left. check left side
      if is_solid(setup_2.left_1) or is_solid(setup_2.left_2) then
        -- make flush with wall
        pn.pos.x := integer_to_f4(tile_to_pixel(setup_2.xn_tile_left + 1));
        -- cancel velocity
        pn.vel.x := integer_to_f4(0);
      end if;
    elsif pn.vel.x > 0 then
      -- going right. check right side
      if is_solid(setup_2.right_1) or is_solid(setup_2.right_2) then
        -- make flush with wall
        pn.pos.x := integer_to_f4(tile_to_pixel(setup_2.xn_tile_right) - PLAYER_WIDTH);
        -- cancel velocity
        pn.vel.x := integer_to_f4(0);
      end if;
    end if;
    if pn.vel.y < 0 then
      -- going down. check bottom
      if is_solid(setup_2.down_1) or is_solid(setup_2.down_2) then
        -- make flush with floor
        pn.pos.y := integer_to_f4(tile_to_pixel(setup_2.yn_tile_down + 1));
        -- cancel velocity
        pn.vel.y := integer_to_f4(0);
      end if;
    elsif pn.vel.y > 0 then
      -- going up. check top
      if is_solid(setup_2.up_1) or is_solid(setup_2.up_2) then
        pn.pos.y := integer_to_f4(tile_to_pixel(setup_2.yn_tile_up) - PLAYER_HEIGHT);
        -- cancel velocity
        pn.vel.y := integer_to_f4(0);
      end if;
    end if;

    -- add to score if touching coin
    if is_touching_coin(p, coin_pos) then
      pn.score := pn.score + POINTS_PER_COIN;
    end if;

    return pn;
  end function;

  function is_touching_coin(p : player_t; coinpos : tilepos_t) return boolean is
    variable x_tile_center : integer;
    variable y_tile_center : integer;
  begin
    -- get the tile that the center of the player is in
    x_tile_center := get_tile_id(to_integer(p.pos.x, fixed_wrap, fixed_truncate) + PLAYER_WIDTH / 2);
    y_tile_center := get_tile_id(to_integer(p.pos.y, fixed_wrap, fixed_truncate) + PLAYER_HEIGHT / 2);

    -- check if the player is touching the coin
    return x_tile_center = coinpos.x and y_tile_center = coinpos.y;
  end function;

  function default_player_setup_1_t return player_setup_1_t is
    variable val : player_setup_1_t := (
      x_tile_left => 0,
      x_tile_right => 0,
      y_tile_down => 0,
      y_tile_up => 0,
      left_tile => (others => '0'),
      right_tile => (others => '0'),
      down_left_tile => (others => '0'),
      down_right_tile => (others => '0'),
      grounded => false
    );
  begin
    return val;
  end function;

  function default_player_setup_2_t return player_setup_2_t is
    variable val : player_setup_2_t := (
      xn_tile_left  => 0,
      xn_tile_right => 0,
      yn_tile_down  => 0,
      yn_tile_up    => 0,
      left_1 => (others => '0'),
      left_2 => (others => '0'),
      right_1 => (others => '0'),
      right_2 => (others => '0'),
      down_1 => (others => '0'),
      down_2 => (others => '0'),
      up_1 => (others => '0'),
      up_2 => (others => '0')
    );
  begin
    return val;
  end function;

  function phase_1_setup(p : player_t; m : tilemap_t) return player_setup_1_t is
    variable s : player_setup_1_t := default_player_setup_1_t;

    variable x_low : integer;
    variable y_low : integer;
  begin
    -- x_low and y_low is the bottom left of the player, in pixels
    x_low := to_integer(p.pos.x, fixed_wrap, fixed_truncate);
    y_low := to_integer(p.pos.y, fixed_wrap, fixed_truncate);

    s.x_tile_left  := get_tile_id(x_low); -- tile x coord containing left side of player
    s.x_tile_right := get_tile_id(x_low + PLAYER_WIDTH - 1); -- tile x coord containing right side of player
    s.y_tile_down  := get_tile_id(y_low); -- tile y coord containing bottom of player
    s.y_tile_up    := get_tile_id(y_low + PLAYER_HEIGHT - 1);

    s.left_tile       := get_tile(m, s.x_tile_left, s.y_tile_down);
    s.right_tile      := get_tile(m, s.x_tile_right, s.y_tile_down);
    s.down_left_tile  := get_tile(m, s.x_tile_left, s.y_tile_down - 1);
    s.down_right_tile := get_tile(m, s.x_tile_right, s.y_tile_down - 1);

    -- TODO: might need another cycle for this
    -- determine if the player is grounded
    if tile_to_pixel(s.y_tile_down) = p.pos.y then
      -- we are on the bottom of a tile, so check if we are on something stand-able...
      if is_solid(s.down_left_tile) or is_solid(s.down_right_tile) then
        -- player is grounded
        s.grounded := true;
      end if;
    end if;

    return s;
  end function;

  function phase_2_setup(p : player_t; m : tilemap_t; setup_1 : player_setup_1_t) return player_setup_2_t is
    variable s      : player_setup_2_t := default_player_setup_2_t;
    variable xn_low : integer;
    variable yn_low : integer;

    variable pn : player_t;

  begin
    -- start with current player state
    pn := p;

    -- save tile coords after integrating velocity
    xn_low := to_integer(pn.pos.x, fixed_wrap, fixed_truncate);
    yn_low := to_integer(pn.pos.y, fixed_wrap, fixed_truncate);

    s.xn_tile_left  := get_tile_id(xn_low);
    s.xn_tile_right := get_tile_id(xn_low + PLAYER_WIDTH - 1);
    s.yn_tile_down  := get_tile_id(yn_low);
    s.yn_tile_up    := get_tile_id(yn_low + PLAYER_HEIGHT - 1);

    s.left_1  := get_tile(m, s.xn_tile_left, setup_1.y_tile_down);
    s.left_2  := get_tile(m, s.xn_tile_left, setup_1.y_tile_up);
    s.right_1 := get_tile(m, s.xn_tile_right, setup_1.y_tile_down);
    s.right_2 := get_tile(m, s.xn_tile_right, setup_1.y_tile_up);
    s.down_1  := get_tile(m, setup_1.x_tile_left, s.yn_tile_down);
    s.down_2  := get_tile(m, setup_1.x_tile_right, s.yn_tile_down);
    s.up_1    := get_tile(m, setup_1.x_tile_left, s.yn_tile_up);
    s.up_2    := get_tile(m, setup_1.x_tile_right, s.yn_tile_up);

    return s;
  end function;

end package body player_funs;
