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
  signal bm_command     : bram_command_t;
  signal bm_read_index  : bram_index_t;
  signal bm_write_index : bram_index_t;

  signal bm_param            : param_t;
  signal bm_param_index      : param_index_t;
  signal bm_param_valid_nn_1 : boolean;
  signal bm_param_valid_nn_2 : boolean;

  signal bm_go   : boolean;
  signal bm_done : boolean;

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
  signal fn_init_playagame : boolean;

  signal fn_playagame_done : boolean;
  signal fn_game_score     : signed(15 downto 0);

  -- playagame (pg) io
  signal pg_swap_start_from_fitness : boolean;
  signal pg_gs                      : gamestate_t;

  -- nn io
  signal nn1_action : playerinput_t;
  signal nn1_go     : boolean;
  signal nn1_done   : boolean;
  signal nn2_action : playerinput_t;
  signal nn2_go     : boolean;
  signal nn2_done   : boolean;

  -- ga io
  signal ga_go               : boolean;
  signal ga_done             : boolean;
  signal ga_rng              : std_logic_vector(31 downto 0);
  signal ga_bm_command       : bram_command_t;
  signal ga_bm_read_index    : bram_index_t;
  signal ga_bm_write_index   : bram_index_t;
  signal ga_bm_mutation_rate : mutation_rate_t;
  signal ga_bm_go            : boolean;

begin

  bram_control_mux_proc : process (all) is
  begin
    if ga_bm_go then
      bm_command     <= ga_bm_command;
      bm_read_index  <= ga_bm_read_index;
      bm_write_index <= ga_bm_write_index;
      bm_go          <= ga_bm_go;
    else
      bm_command     <= fn_bm_command;
      bm_read_index  <= fn_bm_read_index;
      bm_write_index <= (others => '0');
      bm_go          <= fn_bm_go;
    end if;
  end process;

  bram_manager_ent : entity work.bram_manager
    port map (
      clk              => clk,
      command          => bm_command,
      read_index       => bm_read_index,
      write_index      => bm_write_index,
      rng              => ga_rng,
      mutation_rate    => ga_bm_mutation_rate,
      param            => bm_param,
      param_index      => bm_param_index,
      param_valid_nn_1 => bm_param_valid_nn_1,
      param_valid_nn_2 => bm_param_valid_nn_2,
      go               => bm_go,
      done             => bm_done
    );

  tournament_ent : entity work.tournament
    port map (
      clk                      => clk,
      rng                      => ga_rng,
      go                       => tn_go,
      ga_config                => config,
      input_population_fitness => tn_input_population_fitness,
      winner_counts            => tn_winner_counts,
      done                     => tn_done
    );

  fitness_ent : entity work.fitness
    port map (
      clk                       => clk,
      rng                       => ga_rng,
      bm_command                => fn_bm_command,
      bm_read_index             => fn_bm_read_index,
      bm_go                     => fn_bm_go,
      bm_done                   => bm_done,
      ga_config                 => config,
      fitness_go                => fn_go,
      fitness_done              => fn_done,
      seed                      => fn_seed,
      init_playagame            => fn_init_playagame,
      swap_start                => pg_swap_start_from_fitness,
      playagame_done            => fn_playagame_done,
      game_score                => fn_game_score,
      output_population_fitness => tn_input_population_fitness
    );

  playagame_ent : entity work.playagame
    port map (
      clk                     => clk,
      swap_start_from_fitness => pg_swap_start_from_fitness,
      seed_from_fitness       => fn_seed,
      frame_limit             => config.frame_limit,
      game_go                 => fn_init_playagame,
      game_done               => fn_playagame_done,
      score_output            => fn_game_score,
      p1_input                => nn1_action,
      p1_input_valid          => nn1_done,
      p1_request_input        => nn1_go,
      p2_input                => nn2_action,
      p2_input_valid          => nn2_done,
      p2_request_input        => nn2_go,
      gs                      => pg_gs,
      m                       => m
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

  ga_ent : entity work.ga
    port map (
      clk              => clk,
      config           => config,
      go               => ga_go,
      done             => ga_done,
      pause            => false,
      resume           => false,
      rng              => ga_rng,
      bm_command       => ga_bm_command,
      bm_read_index    => ga_bm_read_index,
      bm_write_index   => ga_bm_write_index,
      bm_mutation_rate => ga_bm_mutation_rate,
      bm_go            => ga_bm_go,
      bm_done          => bm_done,
      tn_go            => tn_go,
      tn_done          => tn_done,
      tn_winner_counts => tn_winner_counts,
      fn_go            => fn_go,
      fn_done          => fn_done

    );

end architecture neuroevolution_arch;
