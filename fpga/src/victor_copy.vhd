library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.ga_types.all;
use work.bram_types.all;

entity victor_copy is
  port (
    clk : in std_logic;

    -- config
    config : in ga_config_t;

    -- tournament io
    winner_counts : in winner_counts_array_t;

    -- bram_manager io
    command           : out bram_command_t  := C_COPY_AND_MUTATE;
    read_index        : out bram_index_t    := (others => '0');
    write_index       : out bram_index_t    := (others => '0');
    mutation_rate     : out mutation_rate_t := (others => '0');
    bram_manager_go   : out boolean         := false;
    bram_manager_done : in boolean;

    go   : in boolean;
    done : out boolean := false
  );
end entity victor_copy;

architecture victor_copy_arch of victor_copy is

  type   state_t is (IDLE_S, SEEKING_PTRS_S, COPY_S);
  signal state : state_t := IDLE_S;

  signal winner_counts_r : winner_counts_array_t := default_winner_counts_array_t;

begin

  main_proc : process (all) is

    variable pop_size                   : unsigned(7 downto 0);
    variable read_index_on_multi_victor : boolean := false;
    variable write_index_on_non_victor  : boolean := false;

  begin
    if rising_edge(clk) then
      pop_size := shift_left(to_unsigned(1, 8), to_integer(config.population_size_exp));

      case state is
        when IDLE_S =>
          if go then
            -- reset and register stuff
            winner_counts_r <= winner_counts;
            read_index      <= (others => '0');
            write_index     <= (others => '0');
            done            <= false;
            state           <= SEEKING_PTRS_S;
          end if;
        when SEEKING_PTRS_S =>
          -- we want read_index to be on a multi-victor (victories >= 2), and
          -- we want write_index to be on a non-victor.
          -- when these are both true, we can proceed.

          if read_index < pop_size then
            -- is read_index on a multi-victor?
            read_index_on_multi_victor := winner_counts_r(to_integer(read_index)) >= 2;
            -- is write_index on a non-victor?
            write_index_on_non_victor := winner_counts_r(to_integer(write_index)) = 0;

            if not read_index_on_multi_victor then
              -- increment read_index
              read_index <= read_index + 1;
            end if;
            if not write_index_on_non_victor then
              -- increment or wrap-around write_index
              if write_index = pop_size - 1 then
                write_index <= (others => '0');
              else
                write_index <= write_index + 1;
              end if;
            end if;

            -- if both are good, initiate copy and go to copy state
            if read_index_on_multi_victor and write_index_on_non_victor then
              -- initiate copy
              mutation_rate   <= config.mutation_rates(to_integer(write_index));
              command         <= C_COPY_AND_MUTATE;
              bram_manager_go <= true;
              state           <= COPY_S;

              -- increment non_victor victors to 1
              -- (we already know its 0, so no need for an adder here)
              winner_counts_r(to_integer(write_index)) <= to_unsigned(1, 8);
              -- we also need to decrement the victor count
              -- of the victor that we are copying from
              winner_counts_r(to_integer(read_index)) <= winner_counts_r(to_integer(read_index)) - 1;
            end if;
          else -- we've reached the end, so we're done.
            done        <= true;
            state       <= IDLE_S;
            read_index  <= (others => '0');
            write_index <= (others => '0');
          end if;
        when COPY_S =>
          -- we are just waiting for the copy to finish
          bram_manager_go <= false;
          if bram_manager_done then
            -- copy is done, go back to seeking pointers
            state <= SEEKING_PTRS_S;
          end if;
        when others =>
          null;
      end case;
    end if;
  end process;

end architecture victor_copy_arch;
