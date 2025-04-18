library ieee;

library vunit_lib;
context vunit_lib.vunit_context;

use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.game_types.all;
use work.ga_types.all;

entity tb_fitness is
  generic (
    RUNNER_CFG : string
  );
end entity tb_fitness;

architecture tb of tb_fitness is

  constant CLK_PERIOD : time := 10 ns;

  signal clk                       : std_logic           := '0';
  signal fitness_go                : boolean             := false;
  signal fitness_done              : boolean             := false;
  signal nn1_index                 : unsigned(7 downto 0);
  signal nn2_index                 : unsigned(7 downto 0);
  signal score_output              : signed(15 downto 0) := (others => '0');
  signal output_population_fitness : fitness_array_t     := default_fitness_array_t;

  signal init_playagame : boolean     := false;
  signal frame_go       : boolean     := false;
  signal frame_done_sim : boolean     := false;
  signal game_init      : boolean     := false;
  signal swap_start     : boolean     := false;
  signal gamestate      : gamestate_t := default_gamestate_t;
  signal seed_to_game   : std_logic_vector(31 downto 0);

  signal playagame_done     : boolean := false;
  signal game_phase_counter : integer := 0;
  signal go_latched         : boolean := false;
  signal second_game        : boolean := false;

  constant POPULATION_SIZE : integer := 128;

  signal ga_config : ga_config_t := (
    mutation_rates => default_mutation_rates_t,
    max_gen => to_unsigned(0, 16),
    run_until_stop_cmd => false,
    tournament_size => to_unsigned(5, 8),
    population_size_exp => to_unsigned(7, 8),
    model_history_size => to_unsigned(0, 8),
    model_history_interval => to_unsigned(0, 8),
    seed => (others => '0'),
    reference_count => to_unsigned(0, 8),
    eval_interval => to_unsigned(0, 8),
    seed_count => to_unsigned(0, 8),
    frame_limit => to_unsigned(0, 16)
  );

  function all_nonzero(f : fitness_array_t; size : integer) return boolean is
  begin
    for i in 0 to size - 1 loop
      if f(i) = to_signed(0, 16) then
        return false;
      end if;
    end loop;
    return true;
  end function;

  impure function random_signed return signed is
    variable lfsr : unsigned(15 downto 0) := to_unsigned(now / 1 ns mod 65536, 16);
  begin
    lfsr := lfsr(14 downto 0) & (lfsr(15) xor lfsr(13) xor lfsr(12) xor lfsr(10));
    return to_signed(to_integer(lfsr mod 10), 16);
  end function;

begin

  clk <= not clk after CLK_PERIOD / 2;

  test_runner_watchdog(runner, 10 ms);

  uut_fitness : entity work.fitness
    port map (
      clk                       => clk,
      ga_config                 => ga_config,
      fitness_go                => fitness_go,
      fitness_done              => fitness_done,
      nn1_index                 => nn1_index,
      nn2_index                 => nn2_index,
      seed                      => seed_to_game,
      frame_limit               => open,
      init_playagame            => init_playagame,
      playagame_done            => playagame_done,
      game_score                => score_output,
      output_population_fitness => output_population_fitness
    );

  uut_playagame : entity work.playagame
    port map (
      clk               => clk,
      nn1_index_out     => open,
      nn2_index_out     => open,
      load_nns          => open,
      nns_loaded        => true,
      seed_from_fitness => seed_to_game,
      frame_limit       => ga_config.frame_limit,
      nn1_index_in      => nn1_index,
      nn2_index_in      => nn2_index,
      game_go           => init_playagame,
      game_done         => open,
      score_output      => score_output,
      swap_start        => swap_start,
      seed_to_game      => seed_to_game,
      game_init         => game_init,
      frame_go          => frame_go,
      frame_done        => frame_done_sim,
      gamestate         => gamestate
    );

  game_simulator : process (clk) is
  begin
    if rising_edge(clk) then
      frame_done_sim <= false;

      if game_init then
        game_phase_counter <= 0;
        go_latched         <= false;
        playagame_done     <= false;
        second_game        <= swap_start;
        gamestate.p1.score <= to_signed(0, 16);
      elsif frame_go and not go_latched then
        go_latched         <= true;
        game_phase_counter <= game_phase_counter + 1;

        if game_phase_counter = 3 then
          frame_done_sim     <= true;
          gamestate.p1.score <= random_signed;
          if second_game then
            playagame_done <= true;
          end if;
        else
          frame_done_sim <= true;
        end if;
      elsif not frame_go then
        go_latched <= false;
      end if;
    end if;
  end process;

  main : process is
  begin
    test_runner_setup(runner, RUNNER_CFG);

    wait until rising_edge(clk);
    fitness_go <= true;
    wait until rising_edge(clk);
    fitness_go <= false;

    if run("default") then
      wait until fitness_done;
      wait for CLK_PERIOD;
      check(
            all_nonzero(output_population_fitness, POPULATION_SIZE),
            "All fitness entries should be non-zero"
          );
    end if;

    test_runner_cleanup(runner);
    wait;
  end process;

end architecture tb;
