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
  signal gamestate_send : boolean := false;
  signal tx_ready       : boolean;

  -- game signals
  signal game_done          : boolean;
  signal p_game_done        : boolean := false;
  signal queue_tr_gamestate : boolean := false;

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
      gamestate_send => gamestate_send,
      ready          => tx_ready
    );

  game_ent : entity work.game
    port map (
      clk        => clk,
      init       => inference_go,
      swap_start => false,
      seed       => std_logic_vector(to_unsigned(1, 32)),
      m          => tilemap,
      p1_input   => human_input,
      p2_input   => default_playerinput_t,
      go         => human_input_valid,
      done       => game_done,
      gamestate  => gamestate
    );

  state_proc : process (clk) is
  begin
    if rising_edge(clk) then
      -- default to no sending
      gamestate_send <= false;

      -- get edge of game_done
      p_game_done <= game_done;
      if game_done and not p_game_done then
        -- if we can send it now, then do it
        if tx_ready then
          gamestate_send <= true;
        else
          -- can't do it right away, so queue for it
          queue_tr_gamestate <= true;
        end if;
      end if;

      -- we are queued and ready, so send it
      if queue_tr_gamestate and tx_ready then
        gamestate_send     <= true;
        queue_tr_gamestate <= false;
      end if;
    end if;
  end process;

end architecture core_arch;
