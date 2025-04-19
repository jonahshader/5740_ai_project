library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.game_types.all;

entity playagame is
  port (
    clk : in std_logic;

    -- interface with fitness
    swap_start_from_fitness : in  boolean;                       -- swap start from fitness
    seed_from_fitness       : in  std_logic_vector(31 downto 0); -- seed from fitness.ga_config
    frame_limit             : in  unsigned(15 downto 0);         -- frame_limit from fitness.ga_config
    game_go                 : in  boolean;                       -- start playing a game
    game_done               : out boolean;                       -- goes to fitness
    score_output            : out signed(15 downto 0);           -- final score from player 1 after a single game

    -- interface with players
    p1_input         : in playerinput_t; -- player 1 input from nn1
    p1_input_valid   : in boolean;       -- means that nn1 finished propagation and playerinput is ready
    p1_request_input : out boolean;      -- nn1 start propagation
    p2_input         : in playerinput_t; -- player 2 input from nn2 or human player
    p2_input_valid   : in boolean;       -- means that nn2 finished propagation and playerinput is ready or human player input is read from uart
    p2_request_input : out boolean;      -- nn2 start propagation or read playerinput from uart
    gs               : out gamestate_t;  -- game state to be passed to nns

    m : in tilemap_t -- comes from uart
  );
end entity playagame;

architecture playagame_arch of playagame is

  type   state_t is (
    IDLE_S, LOAD_NNS_S,
    INIT_GAME1_S, WAIT_GAME1_S,
    DONE_S
  );
  signal state : state_t := IDLE_S;

  signal score         : signed(15 downto 0)   := (others => '0');
  signal frame_counter : unsigned(15 downto 0) := (others => '0');

  signal game_init_r : boolean := false;
  signal frame_go_r  : boolean := false;
  signal load_nns_r  : boolean := false;
  signal game_done_r : boolean := false;
  signal frame_done  : boolean := false;

begin

  game_logic_entity : entity work.game
    port map (
      clk        => clk,
      init       => game_init_r,
      swap_start => swap_start_from_fitness,
      seed       => seed_from_fitness,
      m          => m,
      p1_input   => p1_input,
      p2_input   => p2_input,
      go         => frame_go_r,
      done       => frame_done,
      gamestate  => gs
    );

  -- Outputs
  game_done        <= game_done_r;
  score_output     <= score;
  p1_request_input <= load_nns_r;
  p2_request_input <= load_nns_r;

  process (clk) is
  begin
    if rising_edge(clk) then
      game_init_r <= false;
      frame_go_r  <= false;
      load_nns_r  <= false;
      game_done_r <= false;

      case state is
        when IDLE_S =>
          score         <= (others => '0');
          frame_counter <= (others => '0');
          if game_go then
            load_nns_r <= true; -- trigger loading of NN inputs
            state      <= LOAD_NNS_S;
          end if;
        when LOAD_NNS_S =>
          -- wait until both neural network outputs are ready
          if p1_input_valid and p2_input_valid then
            game_init_r <= true;
            state       <= INIT_GAME1_S;
          end if;
        when INIT_GAME1_S =>
          frame_go_r <= true;   -- trigger first game frame
          state      <= WAIT_GAME1_S;
        when WAIT_GAME1_S =>
          if frame_done then
            if frame_limit = 0 or frame_counter < frame_limit - 1 then
              frame_counter <= frame_counter + 1;
              -- only proceed when both inputs are valid
              if p1_input_valid and p2_input_valid then
                frame_go_r <= true;
              end if;
            else
              score       <= gs.p1.score;
              game_done_r <= true;
              state       <= DONE_S;
            end if;
          end if;
        when DONE_S =>
          if not game_go then
            state <= IDLE_S;
          end if;
        when others =>
          state <= IDLE_S;
      end case;
    end if;
  end process;

end architecture playagame_arch;
