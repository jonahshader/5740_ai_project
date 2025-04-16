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

  signal clk : std_logic                     := '0';
  signal go  : boolean                       := false;
  signal rng : std_logic_vector(31 downto 0) := x"00000001";

  signal done_fitness              : boolean         := false;
  signal score_valid               : boolean         := false;
  signal output_population_fitness : fitness_array_t := default_fitness_array_t;

  signal ga_config : ga_config_t := (
    mutation_rates         => default_mutation_rates_t,
    max_gen                => to_unsigned(0, 16),
    run_until_stop_cmd     => false,
    tournament_size        => to_unsigned(2, 8),
    population_size_exp    => to_unsigned(7, 8),
    model_history_size     => to_unsigned(4, 8),
    model_history_interval => to_unsigned(1, 8),
    seed                   => (others => '0'),
    reference_count        => to_unsigned(4, 8),
    eval_interval          => to_unsigned(1, 8),
    seed_count             => to_unsigned(1, 8),
    frame_limit            => to_unsigned(100, 16)
  );

  -- PlayAI interface signals
  signal game_score     : signed(15 downto 0) := (others => '0');
  signal playagame_done : boolean             := false;
  signal init_playagame : boolean             := false;
  signal nn1_index      : unsigned(7 downto 0);
  signal nn2_index      : unsigned(7 downto 0);

  signal init_game  : boolean     := false;
  signal swap_start : boolean     := false;
  signal seed_out   : std_logic_vector(31 downto 0);
  signal go_game    : boolean     := false;
  signal done_game  : boolean     := false;
  signal gamestate  : gamestate_t := default_gamestate_t;

  signal phase_counter : integer := 0;

begin

  clk <= not clk after CLK_PERIOD / 2;

  uut_fitness : entity work.fitness
    port map (
      clk                       => clk,
      go                        => go,
      rng                       => rng,
      game_score                => game_score,
      playagame_done            => playagame_done,
      init_playagame            => init_playagame,
      nn1_index                 => nn1_index,
      nn2_index                 => nn2_index,
      done_fitness              => done_fitness,
      score_valid               => score_valid,
      output_population_fitness => output_population_fitness,
      ga_config                 => ga_config
    );

  uut_playagame : entity work.playagame
    port map (
      clk          => clk,
      init         => init_playagame,
      seed_rng     => rng,
      nn1_index    => nn1_index,
      nn2_index    => nn2_index,
      score_output => game_score,
      done         => playagame_done,
      init_game    => init_game,
      swap_start   => swap_start,
      seed         => seed_out,
      go           => go_game,
      done_game    => done_game,
      gamestate    => gamestate
    );

  -- Simulate internal game FSM completion
  game_process : process (clk) is
  begin
    if rising_edge(clk) then
      if init_game then
        phase_counter      <= 0;
        done_game          <= false;
        gamestate.p1.score <= (others => '0');
      elsif go_game then
        if phase_counter = 3 then
          gamestate.p1.score <= gamestate.p1.score + 3;
          done_game          <= true;
        else
          done_game <= false;
        end if;
        phase_counter <= phase_counter + 1;
      end if;
    end if;
  end process;

  main : process is
  begin
    test_runner_setup(runner, RUNNER_CFG);
    wait until rising_edge(clk);

    if run("default") then
      go <= true;
      wait until rising_edge(clk);
      go <= false;

      wait until done_fitness;

      for i in 0 to 127 loop
        assert output_population_fitness(i) /= to_signed(0, 16)
          report "Fitness score at index " & integer'image(i) & " is zero."
          severity warning;
      end loop;
    end if;

    test_runner_cleanup(runner);
    wait;
  end process;

end architecture tb;
