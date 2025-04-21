-- GA runs the all the steps for the genetic algorithm repeatedly for the desired number of
-- generations, or indefinitely until paused.

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.ga_types.all;
use work.bram_types.all;
use work.custom_utils.all;

entity ga is
  port (
    -- ga io
    clk    : in std_logic;
    config : in ga_config_t;
    go     : in boolean;
    done   : out boolean := false;
    pause  : in boolean;
    resume : in boolean;
    rng    : out std_logic_vector(31 downto 0); -- ga holds onto the global rng
    -- TODO: add outputs for generation fitness

    -- bram_manager (bm) io
    bm_command       : out bram_command_t := C_COPY_AND_MUTATE;
    bm_read_index    : out bram_index_t   := (others => '0');
    bm_write_index   : out bram_index_t   := (others => '0');
    bm_mutation_rate : out mutation_rate_t;
    bm_go            : out boolean        := false;
    bm_done          : in boolean;

    -- tournament (tn) io
    tn_go            : out boolean := false;
    tn_done          : in boolean;
    tn_winner_counts : in winner_counts_array_t;

    -- fitness (fn) io
    fn_go   : out boolean := false;
    fn_done : in boolean
  );
end entity ga;

architecture ga_arch of ga is

  signal pause_queued                : boolean      := false;
  signal prior_best_index            : bram_index_t := (others => '0');
  signal prior_best_interval_counter : bram_index_t := (others => '0');
  signal prior_best_copy_go          : boolean      := false;

  type   state_t is (
    IDLE_S,
    PAUSED_S,
    INIT_BRAM_S,      -- initiate a COPY_AND_MUTATE for a bram init
    RUN_FITNESS_S,
    RUN_TOURNAMENT_S,
    RUN_VICTOR_COPY_S,
    COPY_PRIOR_BEST_S
  );
  signal state : state_t := IDLE_S;

  signal init_bram_counter : bram_index_t          := (others => '0');
  signal current_gen       : unsigned(15 downto 0) := (others => '0');
  signal rng_enable        : boolean;

  -- victor_copy (vc) io
  signal vc_command       : bram_command_t;
  signal vc_read_index    : bram_index_t;
  signal vc_write_index   : bram_index_t;
  signal vc_mutation_rate : mutation_rate_t;
  signal vc_bm_go         : boolean;
  signal vc_go            : boolean := false;
  signal vc_done          : boolean;

  signal init_bram_go               : boolean      := false;
  signal init_bram_read_write_index : bram_index_t := (others => '0');

begin

  rng_enable <= state /= IDLE_S and state /= PAUSED_S;

  xormix32_ent : entity work.xormix32
    port map (
      clk    => clk,
      rst    => to_std_logic(go),
      seed_x => config.seed,
      seed_y => (others => '0'),
      enable => to_std_logic(rng_enable),
      result => rng
    );

  victor_copy_ent : entity work.victor_copy
    port map (
      clk               => clk,
      config            => config,
      winner_counts     => tn_winner_counts,
      command           => vc_command,
      read_index        => vc_read_index,
      write_index       => vc_write_index,
      mutation_rate     => vc_mutation_rate,
      bram_manager_go   => vc_bm_go,
      bram_manager_done => bm_done,
      go                => vc_go,
      done              => vc_done
    );

  bm_mux : process (all) is
  begin
    if vc_bm_go then
      bm_command       <= vc_command;
      bm_read_index    <= vc_read_index;
      bm_write_index   <= vc_write_index;
      bm_mutation_rate <= vc_mutation_rate;
      bm_go            <= vc_bm_go;
    elsif init_bram_go then
      bm_command     <= C_COPY_AND_MUTATE;
      bm_read_index  <= init_bram_read_write_index;
      bm_write_index <= init_bram_read_write_index;
      -- max mutation rate for init
      -- TODO: is this good enough for initialization?
      bm_mutation_rate <= to_unsigned(255, bm_mutation_rate'length);
      bm_go            <= init_bram_go;
    else
      -- prior best copy
      bm_command       <= C_COPY_AND_MUTATE;
      bm_read_index    <= (others => '0');
      bm_write_index   <= prior_best_index;
      bm_mutation_rate <= (others => '0'); -- no mutation, just copy
      bm_go            <= prior_best_copy_go;
    end if;
  end process;

  state_proc : process (all) is

    variable population_size  : unsigned(7 downto 0);
    variable total_brams_used : unsigned(7 downto 0);

  begin
    if rising_edge(clk) then
      -- compute temp var for total brams we need to init
      population_size  := shift_left(to_unsigned(1, 8), to_integer(config.population_size_exp));
      total_brams_used := population_size + config.model_history_size + config.reference_count;

      -- defaults
      done               <= false;
      init_bram_go       <= false;
      fn_go              <= false;
      tn_go              <= false;
      vc_go              <= false;
      prior_best_copy_go <= false;

      -- queue pause
      if pause and state /= PAUSED_S then
        pause_queued <= true;
      end if;

      case state is
        when IDLE_S =>
          if go then
            init_bram_counter <= (others => '0');
            state             <= INIT_BRAM_S;
            prior_best_index  <= population_size;
          end if;
        when INIT_BRAM_S =>
          if bm_done then
            if init_bram_counter = total_brams_used then
              init_bram_counter <= (others => '0');
              -- launch and go to fitness
              fn_go <= true;
              state <= RUN_FITNESS_S;
            else
              -- bram isn't busy, and we have another bram to init,
              -- so initialize it and stay in this state.
              init_bram_go               <= true;
              init_bram_read_write_index <= init_bram_counter;
              init_bram_counter          <= init_bram_counter + 1;
            end if;
          end if;
        when RUN_FITNESS_S =>
          -- wait for it to finish
          if fn_done then
            -- launch and go to tournament
            tn_go <= true;
            state <= RUN_TOURNAMENT_S;
          end if;
        when RUN_TOURNAMENT_S =>
          if tn_done then
            -- launch and go to victor copy
            vc_go <= true;
            state <= RUN_VICTOR_COPY_S;
          end if;
        when RUN_VICTOR_COPY_S =>
          if vc_done then
            -- if pause was queued, go to pause state.
            -- we also pulse done to indicate a successful pause.
            if pause_queued then
              state        <= PAUSED_S;
              pause_queued <= false;
              done         <= true;
            else
              -- record prior best if interval is correct
              if config.model_history_interval - 1 = prior_best_interval_counter then
                prior_best_interval_counter <= (others => '0');
                -- launch and go to copy prior best
                if prior_best_index = population_size + config.model_history_size - 1 then
                  prior_best_index <= population_size;
                else
                  prior_best_index <= prior_best_index + 1;
                end if;
                prior_best_copy_go <= true;
                state              <= COPY_PRIOR_BEST_S;
              else
                prior_best_interval_counter <= prior_best_interval_counter + 1;
                -- launch and go to fitness
                fn_go <= true;
                state <= RUN_FITNESS_S;
              end if;
            end if;
          end if;
        when COPY_PRIOR_BEST_S =>
          if bm_done then
            if current_gen = config.max_gen - 1 and not config.run_until_stop_cmd then
              current_gen <= (others => '0');
              -- pulse done
              done <= true;
              -- go to idle
              state <= IDLE_S;
            else
              -- incr gen
              current_gen <= current_gen + 1;
              -- launch and go to fitness
              fn_go <= true;
              state <= RUN_FITNESS_S;
            end if;
          end if;
        when PAUSED_S =>
          if resume then
            -- on resume, we replicate the transition logic of RUN_VICTOR_COPY_S:

            -- record prior best if interval is correct
            if config.model_history_interval - 1 = prior_best_interval_counter then
              prior_best_interval_counter <= (others => '0');
              -- launch and go to copy prior best
              if prior_best_index = population_size + config.model_history_size - 1 then
                prior_best_index <= population_size;
              else
                prior_best_index <= prior_best_index + 1;
              end if;
              prior_best_copy_go <= true;
              state              <= COPY_PRIOR_BEST_S;
            else
              prior_best_interval_counter <= prior_best_interval_counter + 1;
              -- launch and go to fitness
              fn_go <= true;
              state <= RUN_FITNESS_S;
            end if;
          end if;
        when others =>
          null;
      end case;
    end if;
  end process;

end architecture ga_arch;
