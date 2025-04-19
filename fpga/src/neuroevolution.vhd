library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.bram_types.all;
use work.ga_types.all;
use work.custom_utils.all;
use work.game_types.all;

entity neuroevolution is
  port (
    clk    : in std_logic;
    config : in ga_config_t;
    m      : in tilemap_t
  );
end entity neuroevolution;

architecture neuroevolution_arch of neuroevolution is

  -- bram_manager (bm) io
  signal bm_command       : bram_command_t;
  signal bm_read_index    : bram_index_t;
  signal bm_write_index   : bram_index_t;
  signal bm_mutation_rate : mutation_rate_t;

  signal bm_param            : param_t;
  signal bm_param_index      : param_index_t;
  signal bm_param_valid_nn_1 : boolean;
  signal bm_param_valid_nn_2 : boolean;

  signal bm_go   : boolean;
  signal bm_done : boolean;

  -- xormix32 (xm) io
  signal xm_rst    : boolean;
  signal xm_seed_x : std_logic_vector(31 downto 0);
  signal xm_enable : boolean;
  signal xm_result : std_logic_vector(31 downto 0);

  -- tournament (tn) io
  signal tn_go                       : boolean;
  signal tn_input_population_fitness : fitness_array_t;
  signal tn_winner_counts            : winner_counts_array_t;
  signal tn_done                     : boolean;

  -- fitness (fn) io
  signal fn_bm_command    : bram_command_t;
  signal fn_bm_read_index : bram_index_t;
  signal fn_bm_go         : boolean;

  signal fn_go   : boolean;
  signal fn_done : boolean;

  signal fn_seed           : std_logic_vector(31 downto 0);
  signal fn_frame_limit    : unsigned(15 downto 0);
  signal fn_init_playagame : boolean;

  signal fn_playagame_done : boolean;
  signal fn_game_score     : signed(15 downto 0);

  -- playagame (pg) io
  -- TODO: i dont think fitness currently outputs swap_start
  signal pg_swap_start_from_fitness : boolean;
  signal pg_go                      : boolean;
  signal pg_done                    : boolean;
  signal pg_p1_input                : playerinput_t;
  signal pg_p1_input_valid          : boolean;
  signal pg_p1_request_input        : boolean;
  signal pg_p2_input                : playerinput_t;
  signal pg_p2_input_valid          : boolean;
  signal pg_p2_request_input        : boolean;
  signal pg_gs                      : gamestate_t;

  -- nn io
  signal nn1_action : playerinput_t;
  signal nn1_go     : boolean;
  signal nn1_done   : boolean;
  signal nn2_action : playerinput_t;
  signal nn2_go     : boolean;
  signal nn2_done   : boolean;

begin

  bram_manager_ent : entity work.bram_manager
    port map (
      clk              => clk,
      command          => bm_command,
      read_index       => bm_read_index,
      write_index      => bm_write_index,
      rng              => xm_result,
      mutation_rate    => bm_mutation_rate,
      param            => bm_param,
      param_index      => bm_param_index,
      param_valid_nn_1 => bm_param_valid_nn_1,
      param_valid_nn_2 => bm_param_valid_nn_2,
      go               => bm_go,
      done             => bm_done
    );

  xormix32_ent : entity work.xormix32
    port map (
      clk    => clk,
      rst    => to_std_logic(xm_rst),
      seed_x => xm_seed_x,
      seed_y => (others => '0'),
      enable => to_std_logic(xm_enable),
      result => xm_result
    );

  tournament_ent : entity work.tournament
    port map (
      clk                      => clk,
      rng                      => xm_result,
      go                       => tn_go,
      ga_config                => config,
      input_population_fitness => tn_input_population_fitness,
      winner_counts            => tn_winner_counts,
      done                     => tn_done
    );

  fitness_ent : entity work.fitness
    port map (
      clk                       => clk,
      bm_command                => fn_bm_command,
      bm_read_index             => fn_bm_read_index,
      bm_go                     => fn_bm_go,
      bm_done                   => bm_done,
      ga_config                 => config,
      fitness_go                => fn_go,
      fitness_done              => fn_done,
      seed                      => fn_seed,
      frame_limit               => fn_frame_limit,
      init_playagame            => fn_init_playagame,
      playagame_done            => fn_playagame_done,
      game_score                => fn_game_score,
      output_population_fitness => tn_input_population_fitness
    );

  playagame_ent : entity work.playagame
    port map (
      clk                     => clk,
      swap_start_from_fitness => pg_swap_start_from_fitness,
      seed_from_fitness       => fn_seed,
      -- TODO: why is frame limit coming from fitness?
      --  just take from ga_state
      frame_limit      => fn_frame_limit,
      game_go          => pg_go,
      game_done        => pg_done,
      score_output     => fn_game_score,
      p1_input         => pg_p1_input,
      p1_input_valid   => pg_p1_input_valid,
      p1_request_input => pg_p1_request_input,
      p2_input         => pg_p2_input,
      p2_input_valid   => pg_p2_input_valid,
      p2_request_input => pg_p2_request_input,
      gs               => pg_gs,
      m                => m
    );

  nn1_ent : entity work.nn
    port map (
      clk         => clk,
      param       => bm_param,
      param_index => bm_param_index,
      param_valid => bm_param_valid_nn_1,
      gs          => pg_gs,
      -- TODO: perspective should be a generic
      p1_perspective => true,
      action         => nn1_action,
      go             => nn1_go,
      done           => nn1_done
    );

  nn2_ent : entity work.nn
    port map (
      clk         => clk,
      param       => bm_param,
      param_index => bm_param_index,
      param_valid => bm_param_valid_nn_2,
      gs          => pg_gs,
      -- TODO: perspective should be a generic
      p1_perspective => false,
      action         => nn2_action,
      go             => nn2_go,
      done           => nn2_done
    );

end architecture neuroevolution_arch;
