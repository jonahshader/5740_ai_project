library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.game_types.all;
use work.custom_utils.all;

entity tournament is
  Generic (
    -- tournament architecture parameters
    TOURNAMENT_SIZE : integer := 2;  -- Number of players in the tournament, min of 2
    POPULATION_SIZE : integer := 100;      -- Number of population members
    SCORE_WIDTH : integer := 8;         -- Number of bits to represent the scoure
    INDEX_WIDTH : integer := 8 -- log2(100) = 6.64 Number of bits to represent the index
  );
  port (
    clk : in std_logic;
    rst : in  std_logic;

    -- rng parameters
    seed : in std_logic_vector(31 downto 0); -- seed is 32 bits
    init : in boolean;                       -- re-init the tournament using seed. goes high for one cycle.

    -- tournament population
    input_population_scores : in  STD_LOGIC_VECTOR(POPULATION_SIZE*SCORE_WIDTH-1 downto 0); -- fitnsess of each chromosome
    output_population_winners : out STD_LOGIC_VECTOR(POPULATION_SIZE*INDEX_WIDTH-1 downto 0); -- indices of winners of the tournament

    done : out std_logic
  );
end entity tournament;

architecture tournament_arch of tournament is

  type score_array_t is array(0 to POPULATION_SIZE - 1) of unsigned(SCORE_WIDTH - 1 downto 0);
  type index_array_t is array(0 to POPULATION_SIZE - 1) of unsigned(INDEX_WIDTH - 1 downto 0);

  signal scores  : score_array_t;
  signal winners : index_array_t;

  -- use only 1 RNG stream
  constant NUM_RNG : integer := 1;
  signal rng_output : std_logic_vector(31 downto 0);
  signal enable_rng : boolean := false;

  -- FSM state
  signal count : integer range 0 to POPULATION_SIZE := 0;
  signal state : integer range 0 to 2 := 0;

  -- tournament tracking
  signal tournament_round : integer := 0;
  signal best_score       : unsigned(SCORE_WIDTH - 1 downto 0) := (others => '0');
  signal best_index       : unsigned(INDEX_WIDTH - 1 downto 0) := (others => '0');

begin

  rng : entity work.xormix32
    generic map (
      STREAMS => NUM_RNG
    )
    port map (
      -- clock and reset
      clk => clk,
      rst => to_std_logic(init),

      -- config
      seed_x => seed,
      seed_y => (others => '0'),

      -- rng
      enable => to_std_logic(enable_rng),
      result => rng_output
    );

  -- deserialize input population scores
  scores_gen : for i in 0 to POPULATION_SIZE - 1 generate
    scores(i) <= unsigned(input_population_scores((i + 1) * SCORE_WIDTH - 1 downto i * SCORE_WIDTH));
  end generate;

  -- serialize output winner indices
  output_proc : process(all)
    variable base : integer := 0;
  begin
    base := 0;
    for i in 0 to POPULATION_SIZE - 1 loop
      output_population_winners(base + INDEX_WIDTH - 1 downto base) <= std_logic_vector(winners(i));
      base := base + INDEX_WIDTH;
    end loop;
  end process;

  -- FSM: run one tournament round per clock cycle
  process(clk)
    variable current_index : integer range 0 to POPULATION_SIZE - 1;
    variable score_candidate : unsigned(SCORE_WIDTH - 1 downto 0);
  begin
    if rising_edge(clk) then
      if rst = '1' then
        state <= 0;
        count <= 0;
        done <= '0';
        enable_rng <= FALSE;
      else
        case state is

          when 0 => -- idle
            done <= '0';
            if init = TRUE then
              count <= 0;
              enable_rng <= TRUE;
              state <= 1;
            end if;

          when 1 => -- wait 1 cycle for RNG output
            enable_rng <= FALSE;
            state <= 2;

          when 2 => -- process tournament round
            current_index := to_integer(unsigned(rng_output(INDEX_WIDTH - 1 downto 0)));
            if current_index >= POPULATION_SIZE then
              current_index := POPULATION_SIZE - 1 - (current_index - POPULATION_SIZE); -- truncate & flip high bits
            end if;

            score_candidate := scores(current_index); -- get the score of the current candidate

            if tournament_round = 0 or score_candidate > best_score then
              best_score <= score_candidate;
              best_index <= to_unsigned(current_index, INDEX_WIDTH);
            end if;

            tournament_round <= tournament_round + 1;

            if tournament_round = TOURNAMENT_SIZE - 1 then -- last round
              winners(count) <= best_index; -- store the best index of the tournament
              count <= count + 1; -- increment count to evaluate next tournament
              tournament_round <= 0; -- reset round counter for next tournament
              best_score <= (others => '0'); -- reset best score for next tournament

              if count = POPULATION_SIZE - 1 then -- last tournament (done)
                done <= '1';
                state <= 0; -- go back to idle state
              else
                enable_rng <= TRUE; -- enable RNG for next tournament
                state <= 1;
              end if;
            else -- do another round of the tournament
              enable_rng <= TRUE;
              state <= 1;
            end if;

          when others =>
            state <= 0;

        end case;
      end if;
    end if;
  end process;

end architecture tournament_arch;
