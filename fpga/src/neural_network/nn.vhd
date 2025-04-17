library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;
use ieee.numeric_std.all;
use ieee.fixed_pkg.all;

use work.nn_types.all;
use work.bram_types.all;
use work.game_types.all;

entity nn is
  port (
    clk : in std_logic;

    -- bram io
    param       : in param_t;
    param_index : in param_index_t;
    param_valid : in boolean;

    -- nn io
    gs             : in gamestate_t;
    p1_perspective : in boolean;
    action         : out playerinput_t;
    go             : in boolean;
    done           : out boolean
  );
end entity nn;

architecture nn_arch of nn is

  -- registers
  signal layers        : layers_t             := default_layers_t;
  signal logits        : neuron_logits_t      := default_neuron_logits_t;
  signal layer_counter : unsigned(3 downto 0) := (others => '0');
  signal running       : boolean              := false;

  -- wires
  signal input_logits : neuron_logits_t;

  -- function test return boolean is

  -- begin

  -- end function;

  function observe_state(gs : gamestate_t; p1_perspective : boolean) return neuron_logits_t is
    variable observation : neuron_logits_t := default_neuron_logits_t;
    variable p1          : player_t;
    variable p2          : player_t;
    variable index       : integer         := 0;
  -- need to define a cell_size?
  begin
    -- TODO: figure out how to convert the the value to the proper type - - New Function?
    -- coin_pos
    observation(index) := (gs.coin_pos.x);
    index              := index + 1;
    observation(index) := to_float(gs.coin_pos.y);
    index              := index + 1;

    -- figure out which player is is which based on who is observing
    if p1_perspective then
      p1 := gs.p1;
      p2 := gs.p2;
    else
      p1 := gs.p2;
      p2 := gs.p1;
    end if;

  -- p1 postion

  -- p1 velocity

  -- p2 position

  -- p2 velocity
  end function;

begin

  -- TODO: figure out how to wire up game state to input logits.
  -- also need to zero out the unused ones. might need a function.
  -- input_logits(0) <= gs.coin_pos.x; -- e.g.

  -- TODO: figure out how to wire action to logits
  -- action.jump <= logits(2) > 0; -- e.g.

  main_proc : process (all) is

    variable running_v : boolean;

  begin
    if rising_edge(clk) then
      running_v := running;
      -- default to not done
      done <= false;

      if go then
        running   <= true;
        running_v := true;
      end if;

      if running_v then
        if layer_count = 0 then
          -- first layer input comes from input_logits
          logits        <= layer_forward(layers(to_integer(layer_counter)), input_logits, true);
          layer_counter <= layer_counter + 1;
        elsif layer_counter < LAYER_COUNT - 1 then
          -- hidden layers take in the same logits
          logits        <= layer_forward(layers(to_integer(layer_counter)), logits, true);
          layer_counter <= layer_counter + 1;
        else
          -- output layer doesn't activate
          logits        <= layer_forward(layers(to_integer(layer_counter)), logits, false);
          layer_counter <= to_unsigned(0, layer_counter'length);
          running       <= false;
          -- pulse done
          done <= true;
        end if;
      end if;
    end if;
  end process;

end architecture nn_arch;
