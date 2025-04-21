library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.fixed_pkg.all;
use ieee.fixed_float_types.all;
use ieee.math_real.log2;
use ieee.math_real.ceil;

package ga_types is

  constant MAX_POPULATION_SIZE : integer := 128;
  constant SEED_COUNT_BITS     : integer := 8;
  constant MAX_SEED_COUNT      : integer := 2 ** SEED_COUNT_BITS;

  subtype mutation_rate_t is unsigned(7 downto 0);
  type    mutation_rates_t is array (0 to MAX_POPULATION_SIZE - 1) of mutation_rate_t;
  function default_mutation_rates_t return mutation_rates_t;

  type ga_config_t is record
    mutation_rates         : mutation_rates_t;
    max_gen                : unsigned(15 downto 0);
    run_until_stop_cmd     : boolean;
    tournament_size        : unsigned(7 downto 0);
    population_size_exp    : unsigned(7 downto 0);
    model_history_size     : unsigned(7 downto 0);
    model_history_interval : unsigned(7 downto 0);
    seed                   : std_logic_vector(31 downto 0);
    reference_count        : unsigned(7 downto 0);
    eval_interval          : unsigned(7 downto 0); -- TODO: this is unused

    seed_count  : unsigned(SEED_COUNT_BITS - 1 downto 0); -- seed_count is interpreted from (1 to 2^SEED_COUNT_BITS) not from 0
    frame_limit : unsigned(15 downto 0);
  end record ga_config_t;
  function default_ga_config_t return ga_config_t;

  type ga_state_t is record
    current_gen       : unsigned(15 downto 0);
    reference_fitness : signed(15 downto 0);
  end record ga_state_t;
  function default_ga_state_t return ga_state_t;

  type fitness_array_t is array(0 to MAX_POPULATION_SIZE - 1) of signed(15 downto 0);
  function default_fitness_array_t return fitness_array_t;
  type winner_counts_array_t is array(0 to MAX_POPULATION_SIZE - 1) of unsigned(7 downto 0);
  function default_winner_counts_array_t return winner_counts_array_t;

end package ga_types;

package body ga_types is

  function default_mutation_rates_t return mutation_rates_t is
    variable val : mutation_rates_t := (others => to_unsigned(0, 8));
  begin
    return val;
  end function;

  function default_ga_config_t return ga_config_t is
    variable val : ga_config_t := (
    -- TODO: use reasonable defaults, or require PS to configure?
                                   mutation_rates => default_mutation_rates_t,
                                   max_gen => to_unsigned(0, 16),
                                    run_until_stop_cmd => false,
                                   tournament_size => to_unsigned(0, 8),
                                    population_size_exp => to_unsigned(0, 8),
                                    model_history_size => to_unsigned(0, 8),
                                    model_history_interval => to_unsigned(0, 8),
                                    seed => (others => '0'),
                                    reference_count => to_unsigned(0, 8),
                                    eval_interval => to_unsigned(0, 8),
                                    
                                   seed_count => to_unsigned(0, 8),
                                    frame_limit => to_unsigned(0, 16)
                                  );
  begin
    return val;
  end function;

  function default_ga_state_t return ga_state_t is
    variable val : ga_state_t := (
                                   current_gen => to_unsigned(0, 16),
                                   reference_fitness => to_signed(0, 16)
                                 );
  begin
    return val;
  end function;

  function default_fitness_array_t return fitness_array_t is
    variable val : fitness_array_t := (others => to_signed(0, 16));
  begin
    return val;
  end function;

  function default_winner_counts_array_t return winner_counts_array_t is
    variable val : winner_counts_array_t := (others => to_unsigned(0, 8));
  begin
    return val;
  end function;

end package body ga_types;
