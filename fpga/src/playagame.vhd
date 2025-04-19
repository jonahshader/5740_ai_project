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
    IDLE_S, REQUEST_INPUT_S,
    INIT_GAME1_S, WAIT_GAME1_S
  );
  signal state : state_t := IDLE_S;

  signal score         : signed(15 downto 0)   := (others => '0');
  signal frame_counter : unsigned(15 downto 0) := (others => '0');

  signal game_init_r     : boolean := false;
  signal frame_go_r      : boolean := false;
  signal request_input_r : boolean := false;
  signal game_done_r     : boolean := false;
  signal frame_done      : boolean := false;
  signal game_init_r     : boolean := false;
  signal frame_go_r      : boolean := false;
  signal request_input_r : boolean := false;
  signal game_done_r     : boolean := false;
  signal frame_done      : boolean := false;

  signal p1_input_valid_r : boolean := false;
  signal p2_input_valid_r : boolean := false;
  signal p1_input_r       : playerinput_t;
  signal p2_input_r       : playerinput_t;

  signal p1_input_valid_r : boolean := false;
  signal p2_input_valid_r : boolean := false;
  signal p1_input_r       : playerinput_t;
  signal p2_input_r       : playerinput_t;

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
  p1_request_input <= request_input_r;
  p2_request_input <= request_input_r;
  p1_request_input <= request_input_r;
  p2_request_input <= request_input_r;

  process (clk) is
  begin
    if rising_edge(clk) then
      game_init_r     <= false;
      frame_go_r      <= false;
      request_input_r <= false;
      game_done_r     <= false;

      -- Queue input_valid and latch player inputs
      if p1_input_valid then
        p1_input_valid_r <= true;
        p1_input_r       <= p1_input;
      end if;
      if p2_input_valid then
        p2_input_valid_r <= true;
        p2_input_r       <= p2_input;
      end if;

      case state is
        when IDLE_S =>
          score            <= (others => '0');
          frame_counter    <= (others => '0');
          p1_input_valid_r <= false;
          p2_input_valid_r <= false;
          if game_go then
            request_input_r <= true; -- trigger loading of NN inputs
            state           <= REQUEST_INPUT_S;
          end if;
        when INIT_GAME_S =>
          request_input_r  <= true;
          p1_input_valid_r <= false;
          p2_input_valid_r <= false;
          state            <= WAIT_INPUT_S;
        when WAIT_INPUT_S =>
          if p1_input_valid_r and p2_input_valid_r then
            state <= START_FRAME_S;
          end if;
        when START_FRAME_S =>
          frame_go_r       <= true;
          p1_input_valid_r <= false;
          p2_input_valid_r <= false;
          state            <= WAIT_FRAME_DONE_S;
        when WAIT_FRAME_DONE_S =>
          if frame_done then
            if frame_limit = 0 or frame_counter < frame_limit - 1 then
              frame_counter   <= frame_counter + 1;
              request_input_r <= true;
              state           <= WAIT_INPUT_S;
            else
              score       <= gs.p1.score;
              game_done_r <= true;
              state       <= IDLE_S;
              state       <= IDLE_S;
            end if;
          end if;
        when others =>
          state <= IDLE_S;
      end case;
    end if;
  end process;

end architecture playagame_arch;
