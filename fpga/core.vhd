library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.game_types.all;
use work.ga_types.all;

entity core is
  port (
    clk : in std_logic;

    o_rx_dv   : in std_logic;
    o_rx_byte : in std_logic_vector(7 downto 0);

    i_tx_dv   : out std_logic;
    i_tx_byte : out std_logic_vector(7 downto 0);
    o_tx_done : in std_logic
  );
end entity core;

architecture core_arch of core is

  -- comms_rx signals
  signal training_go       : boolean;
  signal training_stop     : boolean;
  signal inference_go      : boolean;
  signal human_input       : playerinput_t;
  signal human_input_valid : boolean;
  signal tilemap           : tilemap_t;
  signal ga_config         : ga_config_t;

  -- comms_tx signals
  signal ga_state       : ga_state_t;
  signal ga_state_send  : boolean;
  signal gamestate      : gamestate_t;
  signal gamestate_send : boolean;

begin

  comms_rx_ent : entity work.comms_rx
    port map (
      clk               => clk,
      uart_rx           => o_rx_byte,
      uart_rx_valid     => o_rx_dv,
      training_go       => training_go,
      training_stop     => training_stop,
      inference_go      => inference_go,
      human_input       => human_input,
      human_input_valid => human_input_valid,
      tilemap           => tilemap,
      ga_config         => ga_config
    );

  comms_tx_ent : entity work.comms_tx
    port map (
      clk            => clk,
      uart_tx        => i_tx_byte,
      uart_tx_send   => i_tx_dv,
      uart_done      => o_tx_done,
      ga_state       => ga_state,
      ga_state_send  => ga_state_send,
      gamestate      => gamestate,
      gamestate_send => gamestate_send
    );

end architecture core_arch;
