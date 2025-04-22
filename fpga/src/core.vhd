library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.game_types.all;
use work.ga_types.all;
use work.ne_types.all;

entity core is
  port (
    clk : in std_logic;

    o_rx_dv   : in std_logic;
    o_rx_byte : in std_logic_vector(7 downto 0);

    i_tx_dv   : out std_logic;
    i_tx_byte : out std_logic_vector(7 downto 0);
    o_tx_done : in std_logic;

    led_out : out std_logic
  );
end entity core;

architecture core_arch of core is

  -- comms_rx signals
  signal training_go       : boolean;
  signal training_pause    : boolean;
  signal training_resume   : boolean;
  signal inference_go      : boolean;
  signal inference_stop    : boolean;
  signal human_input       : playerinput_t;
  signal human_input_valid : boolean;
  signal test_go           : boolean;
  signal tilemap           : tilemap_t;
  signal ga_config         : ga_config_t;
  signal play_against_nn   : boolean;

  -- comms_tx signals
  signal ga_state      : ga_state_t;
  signal ga_state_send : boolean;
  signal gamestate     : gamestate_t;
  signal tx_ready      : boolean;

  -- neuroevolution signals
  signal announce_new_state : boolean;
  signal ne_state           : ne_state_t;
  signal transmit_gs        : boolean;

  signal led_counter : unsigned(25 downto 0) := to_unsigned(0, 26);

begin

  led_out <= led_counter(25);

  ne_ent : entity work.neuroevolution
    port map (
      clk                => clk,
      config             => ga_config,
      m                  => tilemap,
      training_go        => training_go,
      training_pause     => training_pause,
      training_resume    => training_resume,
      inference_go       => inference_go,
      inference_stop     => inference_stop,
      human_input        => human_input,
      human_input_valid  => human_input_valid,
      play_against_nn    => play_against_nn,
      announce_new_state => announce_new_state,
      state              => ne_state,
      transmit_gs        => transmit_gs
    );

  comms_rx_ent : entity work.comms_rx
    port map (
      clk               => clk,
      uart_rx           => o_rx_byte,
      uart_rx_valid     => o_rx_dv,
      training_go       => training_go,
      training_pause    => training_pause,
      training_resume   => training_resume,
      inference_go      => inference_go,
      inference_stop    => inference_stop,
      human_input       => human_input,
      human_input_valid => human_input_valid,
      test_go           => test_go,
      tilemap           => tilemap,
      ga_config         => ga_config,
      play_against_nn   => play_against_nn
    );

  comms_tx_ent : entity work.comms_tx
    port map (
      clk                   => clk,
      uart_tx               => i_tx_byte,
      uart_tx_send          => i_tx_dv,
      uart_done             => o_tx_done,
      ga_state              => ga_state,
      ga_state_send         => ga_state_send,
      gamestate             => gamestate,
      gamestate_send        => transmit_gs,
      test_go               => test_go,
      ne_announce_new_state => announce_new_state,
      ne_state              => ne_state,
      ready                 => tx_ready
    );

  led_proc : process (clk) is
  begin
    if rising_edge(clk) then
      -- counter for led sanity check
      if led_counter = (2 ** 26) - 1 then
        led_counter <= to_unsigned(0, 26);
      else
        led_counter <= led_counter + 1;
      end if;
    end if;
  end process;

end architecture core_arch;
