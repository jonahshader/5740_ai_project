library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library vunit_lib;
context vunit_lib.vunit_context;

use work.bram_types.all;
use work.ga_types.all;
use work.custom_utils.all;
use work.game_types.all;
use work.ne_types.all;

entity tb_neuroevolution is
  generic (
    RUNNER_CFG : string
  );
end entity tb_neuroevolution;

architecture tb of tb_neuroevolution is

  constant CLK_100MHZ_PERIOD : time      := 10 ns;
  signal   clk               : std_logic := '0';

  signal config : ga_config_t := default_ga_config_t; -- TODO: override defaults
  signal m      : tilemap_t   := test_tilemap_t;

  signal training_go     : boolean := false;
  signal training_pause  : boolean := false;
  signal training_resume : boolean := false;
  signal inference_go    : boolean := false;
  signal inference_stop  : boolean := false;

  signal human_input       : playerinput_t := default_playerinput_t;
  signal human_input_valid : boolean       := false;
  signal play_against_nn   : boolean       := false;

  signal announce_new_state : boolean;
  signal state              : ne_state_t;
  signal transmit_gs        : boolean;

begin

  -- Timeout after 10 ms.
  test_runner_watchdog(runner, 10 ms);

  clk <= not clk after CLK_100MHZ_PERIOD / 2;

  ne_inst : entity work.neuroevolution
    port map (
      clk                => clk,
      config             => config,
      m                  => m,
      training_go        => training_go,
      training_pause     => training_pause,
      training_resume    => training_resume,
      inference_go       => inference_go,
      inference_stop     => inference_stop,
      human_input        => human_input,
      human_input_valid  => human_input_valid,
      play_against_nn    => play_against_nn,
      announce_new_state => announce_new_state,
      state              => state,
      transmit_gs        => transmit_gs
    );

  test_process : process is
  begin
    test_runner_setup(runner, RUNNER_CFG);

    while test_suite loop
      wait until rising_edge(clk);

      if run("train") then
        -- configure
        config.mutation_rates         <= (others => to_unsigned(128, 8));
        config.max_gen                <= to_unsigned(1, 16);
        config.tournament_size        <= to_unsigned(2, 8);
        config.population_size_exp    <= to_unsigned(2, 8);
        config.model_history_size     <= to_unsigned(2, 8);
        config.model_history_interval <= to_unsigned(1, 8);
        config.seed                   <= x"8DF9B76F";
        config.reference_count        <= to_unsigned(2, 8);
        -- config.eval_interval
        config.seed_count  <= to_unsigned(2, 8);
        config.frame_limit <= to_unsigned(10, 16);

        check(state = NE_IDLE_S, "NE state should be idle, but is " & to_string(state));

        wait until rising_edge(clk);
        training_go <= true;
        wait until rising_edge(clk);
        training_go <= false;

        wait until announce_new_state;
        -- we should be training
        check(state = NE_TRAINING_S, "NE state should be training, but is " & to_string(state));
        wait until announce_new_state;
        -- we should go back to idle
        check(state = NE_IDLE_S, "NE state should be idle, but is " & to_string(state));
      end if;
    end loop;

    test_runner_cleanup(runner);
  end process;

end architecture tb;
