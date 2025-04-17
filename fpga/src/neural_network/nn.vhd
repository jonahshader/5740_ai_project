library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;
use ieee.numeric_std.all;
use ieee.fixed_pkg.all;
use ieee.fixed_float_types.all;

use work.nn_types.all;
use work.bram_types.all;
use work.game_types.all;
use work.decoder_funs.all;

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

  function observe_state(gs : gamestate_t; p1_perspective : boolean) return neuron_logits_t is
    variable observation : neuron_logits_t := default_neuron_logits_t;
    variable index       : integer         := 0;
    variable first       : player_t;
    variable second      : player_t;

  begin
    -- coin_pos
    observation(index) := signed(shift_left(resize(gs.coin_pos.x, observation(0)'length), TILE_PX_BITS));
    index              := index + 1;
    observation(index) := signed(shift_left(resize(gs.coin_pos.y, observation(0)'length), TILE_PX_BITS));
    index              := index + 1;

    -- figure out which player is which based on who is observing
    if p1_perspective then
      first  := gs.p1;
      second := gs.p2;
    else
      first  := gs.p2;
      second := gs.p1;
    end if;

    -- first position
    observation(index) := to_signed(first.pos.x, observation(0)'length, fixed_wrap, fixed_truncate);
    index              := index + 1;
    observation(index) := to_signed(first.pos.y, observation(0)'length, fixed_wrap, fixed_truncate);
    index              := index + 1;
    -- first velocity
    observation(index) := resize(signed(to_slv(first.vel.x)), observation(0)'length);
    index              := index + 1;
    observation(index) := resize(signed(to_slv(first.vel.y)), observation(0)'length);
    index              := index + 1;
    -- first dead flag
    observation(index) := to_signed(32, observation(0)'length) when first.dead_timeout = 0 else to_signed(-32, observation(0)'length);
    index              := index + 1;

    -- second position
    observation(index) := to_signed(second.pos.x, observation(0)'length, fixed_wrap, fixed_truncate);
    index              := index + 1;
    observation(index) := to_signed(second.pos.y, observation(0)'length, fixed_wrap, fixed_truncate);
    index              := index + 1;
    -- second velocity
    observation(index) := resize(signed(to_slv(second.vel.x)), observation(0)'length);
    index              := index + 1;
    observation(index) := resize(signed(to_slv(second.vel.y)), observation(0)'length);
    index              := index + 1;
    -- second dead flag
    observation(index) := to_signed(32, observation(0)'length) when second.dead_timeout = 0 else to_signed(-32, observation(0)'length);
    index              := index + 1;

    return observation;
  end function;

begin

  -- wire up game state to input logits.
  input_logits <= observe_state(gs, p1_perspective);

  -- wire action to logits
  -- note: this is different than the c++ implementation,
  -- which does multiple thresholds on output 2 to determine {left, none, right}
  -- here it is better to interpret separate outputs because we don't have a
  -- point of reference for the output scale, so we can't establish non-zero
  -- thresholds easily.
  action.left  <= logits(0) > 0;
  action.right <= logits(1) > 0;
  action.jump  <= logits(2) > 0;

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

  -- decoder process. kept separate from main_proc for clarity.
  decoder_proc : process (all) is
  begin
    if rising_edge(clk) then
      if param_valid then
        -- when we receive a valid parameter, we need to place
        -- it into the proper location. this is what decode_address
        -- does. see decoder_funs.vhd
        layers <= decode_address(layers, param, param_index);
      end if;
    end if;
  end process;

end architecture nn_arch;
