library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.fixed_pkg.all;
use ieee.fixed_float_types.all;
use ieee.math_real.log2;
use ieee.math_real.ceil;
use work.game_types.all;

package player_funs is

  function phase_1(p : player_t; other : player_t) return player_t;

end package player_funs;

package body player_funs is

  function phase_1(p : player_t; other : player_t) return player_t is
    variable updated_player : player_t;
  begin
    -- start with current player state
    updated_player := p;
  end function;

end package body player_funs;
