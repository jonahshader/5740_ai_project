library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.fixed_pkg.all;
use ieee.fixed_float_types.all;
use ieee.math_real.log2;
use ieee.math_real.ceil;

package ga_types is

  constant MAX_POPULATION_SIZE : integer := 128;

  subtype mutation_rate_t is unsigned(7 downto 0);
  type    mutation_rates_t is array (0 to MAX_POPULATION_SIZE - 1) of mutation_rate_t;
  function default_mutation_rates_t return mutation_rates_t;

  type ga_config_t is record
    mutation_rates         : mutation_rates_t;
    max_gen                : unsigned(15 downto 0);
    run_until_stop_cmd     : boolean;
    tournament_size        : unsigned(7 downto 0);
    population_size        : unsigned(7 downto 0);
    model_history_size     : unsigned(7 downto 0);
    model_history_interval : unsigned(7 downto 0);
    seed                   : std_logic_vector(31 downto 0);
    reference_count        : unsigned(7 downto 0);
    eval_interval          : unsigned(7 downto 0);

    seed_count  : unsigned(7 downto 0);
    frame_limit : unsigned(15 downto 0);
  end record ga_config_t;
  function default_ga_config_t return ga_config_t;

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
      population_size => to_unsigned(0, 8),
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

end package body ga_types;
