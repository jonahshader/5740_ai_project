library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

package ne_types is

  type ne_state_t is (NE_IDLE_S, NE_TRAINING_S, NE_PLAYING_S);

end package ne_types;
