-- the bram manager houses all the brams, which store the
-- neural network parameters. the bram manager is responsible
-- for copying neural nets to other brams, copy & mutate,
-- and randomly initializing.

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.math_real.log2;
use ieee.math_real.ceil;

use work.bram_types.all;
use work.nn_types.total_params;
use work.ga_types.mutation_rate_t;
use work.mutate_funs.all;

entity bram_manager is
  port (
    clk           : in std_logic;
    command       : in bram_command_t;
    read_index    : in bram_index_t;
    write_index   : in bram_index_t;
    rng           : in std_logic_vector(31 downto 0);
    mutation_rate : in mutation_rate_t;

    param            : out param_t       := (others => '0');
    param_index      : out param_index_t := to_unsigned(0, 14);
    param_valid_nn_1 : out boolean       := false;
    param_valid_nn_2 : out boolean       := false;

    go   : in boolean;
    done : out boolean := true
  );
end entity bram_manager;

architecture bram_manager_arch of bram_manager is

  type    dout_b_arr_t is array (0 to NUM_BRAMS - 1) of param_t;
  type    we_a_arr_t is array (0 to NUM_BRAMS - 1) of boolean;
  subtype addr_t is unsigned(integer(ceil(log2(real(BRAM_DEPTH)))) - 1 downto 0);

  signal we_a_arr   : we_a_arr_t := (others => false);
  signal dout_b_arr : dout_b_arr_t;

  signal addr_a : addr_t := (others => '0');
  signal din_a  : param_t;

  signal command_r       : bram_command_t  := C_COPY_AND_MUTATE;
  signal read_index_r    : bram_index_t    := (others => '0');
  signal write_index_r   : bram_index_t    := (others => '0');
  signal mutation_rate_r : mutation_rate_t := (others => '0');

begin

  param            <= dout_b_arr(to_integer(read_index_r));
  param_valid_nn_1 <= command_r = C_READ_TO_NN_1 and not done;
  param_valid_nn_2 <= command_r = C_READ_TO_NN_2 and not done;

  bram_gen : for i in 0 to NUM_BRAMS - 1 generate
    bram_inst : entity work.bram_sdp
      generic map (
        WIDTH => BRAM_WIDTH,
        DEPTH => BRAM_DEPTH
      )
      port map (
        -- port a (write only)
        clk_a  => clk,
        we_a   => we_a_arr(i),
        addr_a => addr_a,
        din_a  => din_a,
        -- port b (read only)
        clk_b  => clk,
        en_b   => true, -- always reading
        addr_b => param_index,
        dout_b => dout_b_arr(i)
      );
  end generate bram_gen;

  proc : process (all) is
  begin
    if rising_edge(clk) then
      if done then -- not running, able to accept command
        if go then -- accept command
          done            <= false;
          command_r       <= command;
          read_index_r    <= read_index;
          write_index_r   <= write_index;
          mutation_rate_r <= mutation_rate;
          param_index     <= to_unsigned(0, param_index'length);
          addr_a          <= to_unsigned(0, addr_a'length);
          done            <= false;
          we_a_arr        <= (others => false);
        end if;
      else         -- running
        case command_r is
          when C_COPY_AND_MUTATE =>
            if param_index < TOTAL_PARAMS - 1 then
              param_index <= param_index + 1;
              din_a       <= mutate_param(param, param_index, rng, mutation_rate_r);
            else
              param_index <= to_unsigned(0, param_index'length);
              done        <= true;
            end if;
            addr_a                              <= param_index;
            we_a_arr(to_integer(write_index_r)) <= true;
          when C_READ_TO_NN_1 | C_READ_TO_NN_2 =>
            if param_index < TOTAL_PARAMS - 1 then
              param_index <= param_index + 1;
            else   -- addr_a = TOTAL_PARAMS - 1, the last index
              param_index <= to_unsigned(0, param_index'length);
              done        <= true;
            end if;
          when others =>
            null;
        end case;
      end if;
    end if;
  end process;

end architecture bram_manager_arch;
