library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.fixed_pkg.all;
use ieee.fixed_float_types.all;
use ieee.math_real.log2;
use ieee.math_real.ceil;
use work.game_types.all;

package player_funs is

  function phase_1(p : player_t; other : player_t; input : playerinput_t; m : tilemap_t) return player_t;
  function phase_2(p : player_t; p_spawn : tilepos_t; coin_pos : tilepos_t; m : tilemap_t) return player_t;
  function is_touching_coin(p : player_t; coinpos : tilepos_t) return boolean;

end package player_funs;

package body player_funs is

  -- Phase 1
  function phase_1(p : player_t; other : player_t; input : playerinput_t; m : tilemap_t) return player_t is
    variable pn      : player_t;
    variable other_p : player_t;

    variable x_low : integer;
    variable y_low : integer;

    variable x_tile_left  : integer;
    variable x_tile_right : integer;
    variable y_tile_down  : integer;
    variable y_tile_up    : integer;

    variable left_tile       : tile_t;
    variable right_tile      : tile_t;
    variable down_left_tile  : tile_t;
    variable down_right_tile : tile_t;

    variable grounded : boolean := false;
    variable in_water : boolean;
    variable on_ice   : boolean;

    variable grav     : f4_t := GRAVITY;
    variable move_acc : f4_t := MOVE_ACCEL;

    variable accel : boolean := true;
  begin
    -- start with current player state
    pn      := p;
    other_p := other;

    -- x_low and y_low is the bottom left of the player, in pixels
    x_low := to_integer(pn.pos.x, fixed_wrap, fixed_truncate);
    y_low := to_integer(pn.pos.y, fixed_wrap, fixed_truncate);

    x_tile_left  := get_tile_id(x_low); -- tile x coord containing left side of player
    x_tile_right := get_tile_id(x_low + PLAYER_WIDTH - 1); -- tile x coord containing right side of player
    y_tile_down  := get_tile_id(y_low); -- tile y coord containing bottom of player
    y_tile_up    := get_tile_id(y_low + PLAYER_HEIGHT - 1); -- tile y coord containing top of player

    left_tile       := get_tile(m, x_tile_left, y_tile_down);
    right_tile      := get_tile(m, x_tile_right, y_tile_down);
    down_left_tile  := get_tile(m, x_tile_left, y_tile_up - 1);
    down_right_tile := get_tile(m, x_tile_right, y_tile_up - 1);

    -- early return if dead
    if pn.dead_timeout > 0 then
      return pn;
    end if;

    -- determine if the player is grounded
    if tile_to_pixel(y_tile_down) = pn.pos.y then
      -- we are on the bottom of a tile, so check if we are on something stand-able...
      if is_solid(down_left_tile) or is_solid(down_right_tile) then
        -- player is grounded
        grounded := true;
      end if;
    end if;

    -- determine if in water
    in_water := is_water(left_tile) or is_water(right_tile);
    on_ice   := down_left_tile = TILE_ICE or down_right_tile = TILE_ICE;

    -- determine acceleration based on context
    grav     := GRAVITY_WATER when in_water else gravity;
    move_acc := MOVE_ACCEL_ICE when on_ice else MOVE_ACCEL_WATER when in_water else move_accel;

    -- jump logic
    if grounded then
      if down_left_tile = TILE_SPRING or down_right_tile = TILE_SPRING then
        -- spring tile
        pn.vel.y := SPRING_VEL;
      elsif input.jump then
        -- jump
        pn.vel.y := JUMP_VEL;
      end if;
    else -- not grounded. accelerate due to gravity
      pn.vel.y := pn.vel.y + grav;
      if input.jump then
        -- if jump is held, also accelerate up a little to 'float'
        pn.vel.y := pn.vel.y + JUMP_MIDAIR_ACCEL;
      end if;
      -- limit y_vel
      -- TODO: might save circuits to pull this out of the if statement
      if pn.vel.y > FALL_MAX_VEL then
        pn.vel.y := FALL_MAX_VEL;
      end if;
    end if;

    -- accelerate x_vel based on input
    if input.left and not input.right then
      -- accel left
      pn.vel.x := pn.vel.x - move_acc;
    elsif input.right and not input.left then
      -- accel right
      pn.vel.x := pn.vel.x + move_acc;
    else
      -- decelerate towards zero if grounded and not on ice
      if grounded and not on_ice then
        if pn.vel.x > 0 then
          -- check if we have room to do the full speed reduction
          if pn.vel.x >= move_acc then
            pn.vel.x := pn.vel.x - move_acc;
          else
            -- we are too slow to do the full speed reduction
            pn.vel.x := integer_to_f4(0);
          end if;
        elsif pn.vel.x < 0 then
          -- check if we have room to do the full speed reduction
          if pn.vel.x <= -move_acc then
            pn.vel.x := pn.vel.x + move_acc;
          else
            -- we are too slow to do the full speed reduction
            pn.vel.x := integer_to_f4(0);
          end if;
        end if;
      end if;
    end if;

    -- limit x_vel
    if pn.vel.x < -MOVE_MAX_VEL then
      pn.vel.x := -MOVE_MAX_VEL;
    end if;
    if pn.vel.x > MOVE_MAX_VEL then
      pn.vel.x := MOVE_MAX_VEL;
    end if;

    -- accelerate based on collision with other player
    if other_p.dead_timeout = 0 then -- only run when other guy is alive
      if abs(pn.pos.y - other_p.pos.y) < PLAYER_HEIGHT then -- if overlapping in y axis
        if abs(pn.pos.x - other_p.pos.x) <= PLAYER_WIDTH then -- if overlapping in x axis
          -- if other player is significantly above this one, die
          if other_p.pos.y >= p.pos.y + PLAYER_KILL_HEIGHT then
            pn.dead_timeout := to_unsigned(DEAD_TIMEOUT, pn.dead_timeout'length);
          -- if the opposite is true, gain a point
          elsif pn.pos.y >= other_p.pos.y + PLAYER_KILL_HEIGHT then
            pn.score := pn.score + POINTS_PER_KILL;
            accel    := false; -- don't accel from collision if it results in a kill
          end if;

          if accel then
            if pn.pos.x > other_p.pos.x then
              pn.vel.x := pn.pos.x - (pn.pos.x - other_p.pos.x - PLAYER_WIDTH);
            elsif pn.pos.x < other_p.pos.x then
              pn.vel.x := pn.pos.x - (pn.pos.x - other_p.pos.x + PLAYER_WIDTH);
            end if;
          end if;
        end if;
      end if;
    end if;

    return pn;
  end function;

  -- Phase 2
  function phase_2(p : player_t; p_spawn : tilepos_t; coin_pos : tilepos_t; m : tilemap_t) return player_t is
    variable pn : player_t;

    variable xn_low : integer;
    variable yn_low : integer;

    variable x_low : integer;
    variable y_low : integer;

    variable xn_tile_left  : integer;
    variable xn_tile_right : integer;
    variable yn_tile_down  : integer;
    variable yn_tile_up    : integer;

    variable x_tile_left  : integer;
    variable x_tile_right : integer;
    variable y_tile_down  : integer;
    variable y_tile_up    : integer;

    variable left_1  : tile_t;
    variable left_2  : tile_t;
    variable right_1 : tile_t;
    variable right_2 : tile_t;
    variable down_1  : tile_t;
    variable down_2  : tile_t;
    variable up_1    : tile_t;
    variable up_2    : tile_t;

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

    -- save tile coords before integrating velocity

    -- x_low and y_low is the bottom left of the player, in pixels
    x_low := to_integer(pn.pos.x, fixed_wrap, fixed_truncate);
    y_low := to_integer(pn.pos.y, fixed_wrap, fixed_truncate);

    x_tile_left  := get_tile_id(x_low); -- tile x coord containing left side of player
    x_tile_right := get_tile_id(x_low + PLAYER_WIDTH - 1); -- tile x coord containing right side of player
    y_tile_down  := get_tile_id(y_low); -- tile y coord containing bottom of player
    y_tile_up    := get_tile_id(y_low + PLAYER_HEIGHT - 1); -- tile y coord containing top of player

    -- integrate velocity
    pn.pos.x := pn.pos.x + pn.vel.x;
    pn.pos.y := pn.pos.y + pn.vel.y;

    -- save tile coords after integrating velocity
    xn_low := to_integer(pn.pos.x, fixed_wrap, fixed_truncate);
    yn_low := to_integer(pn.pos.y, fixed_wrap, fixed_truncate);

    xn_tile_left  := get_tile_id(xn_low);
    xn_tile_right := get_tile_id(xn_low + PLAYER_WIDTH - 1);
    yn_tile_down  := get_tile_id(yn_low);
    yn_tile_up    := get_tile_id(yn_low + PLAYER_HEIGHT - 1);

    -- handle left right collisions
    if pn.vel.x < 0 then
      -- going left. check left side
      left_1 := get_tile(m, xn_tile_left, y_tile_down);
      left_2 := get_tile(m, xn_tile_left, y_tile_up);
      if is_solid(left_1) or is_solid(left_2) then
        -- make flush with wall
        pn.pos.x := integer_to_f4(tile_to_pixel(xn_tile_left + 1));
        -- cancel velocity
        pn.vel.x := integer_to_f4(0);
      end if;
    elsif pn.vel.x > 0 then
      -- going right. check right side
      right_1 := get_tile(m, xn_tile_right, y_tile_down);
      right_2 := get_tile(m, xn_tile_right, y_tile_up);
      if is_solid(right_1) or is_solid(right_2) then
        -- make flush with wall
        pn.pos.x := integer_to_f4(tile_to_pixel(xn_tile_right) - PLAYER_WIDTH);
        -- cancel velocity
        pn.vel.x := integer_to_f4(0);
      end if;
    end if;
    if pn.vel.y < 0 then
      -- going down. check bottom
      down_1 := get_tile(m, x_tile_left, yn_tile_down);
      down_2 := get_tile(m, x_tile_right, yn_tile_down);
      if is_solid(down_1) or is_solid(down_2) then
        -- make flush with floor
        pn.pos.y := integer_to_f4(tile_to_pixel(yn_tile_down + 1));
        -- cancel velocity
        pn.vel.y := integer_to_f4(0);
      end if;
    elsif pn.vel.y > 0 then
      -- going up. check top
      up_1 := get_tile(m, x_tile_left, yn_tile_up);
      up_2 := get_tile(m, x_tile_right, yn_tile_up);
      if is_solid(up_1) or is_solid(up_2) then
        pn.pos.y := integer_to_f4(tile_to_pixel(yn_tile_up) - PLAYER_HEIGHT);
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

end package body player_funs;
