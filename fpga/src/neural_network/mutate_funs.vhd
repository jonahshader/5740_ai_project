library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.math_real.log2;
use ieee.math_real.ceil;

use work.bram_types.param_t;
use work.ga_types.mutation_rate_t;

package mutate_funs is

  function mutate_param(param : param_t; rng : std_logic_vector(31 downto 0); mutation_rate : mutation_rate_t; bias : boolean) return param_t;

end package mutate_funs;

package body mutate_funs is

  function mutate_param(param : param_t; rng : std_logic_vector(31 downto 0); mutation_rate : mutation_rate_t; bias : boolean) return param_t is
     -- 2 extra bits for overflow handling
    variable m_param : signed(param'length downto 0) := resize(signed(param), param'length + 2);
  begin
    -- check for mutation
    if unsigned(rng(7 downto 0)) <= mutation_rate then
      -- we are gonna mutate.
      -- make a uniform mutation
      -- TODO: rewrite this in a less dumb way.
      -- can just add a mutation signed to the signed param.
      -- but the mutation must be restricted and non-zero
      -- before adding.
      case unsigned(rng(10 downto 8)) is
        when to_unsigned(0, 3) =>
          m_param := m_param - 1;
        when to_unsigned(1, 3) =>
          m_param := m_param + 1;
        when to_unsigned(2, 3) =>
          m_param := m_param - 2;
        when to_unsigned(3, 3) =>
          m_param := m_param + 2;
        when to_unsigned(4, 3) =>
          m_param := m_param - 3;
        when to_unsigned(5, 3) =>
          m_param := m_param + 3;
        when to_unsigned(6, 3) =>
          m_param := m_param - 4;
        when to_unsigned(7, 3) =>
          m_param := m_param + 4;
        when others =>
        null;    
      end case;

      -- clamp
      if m_param < to_signed(-2, m_param'length) then
        m_param := to_signed(-2, m_param'length);
      elsif m_param > to_signed(2, m_param'length) then
        m_param := to_signed(2, m_param'length);
      end if;
    end if;

    return std_logic_vector(resize(m_param, param'length));
  end function;


end package body mutate_funs;