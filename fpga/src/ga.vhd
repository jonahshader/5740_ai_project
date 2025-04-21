library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.ga_types.all;
use work.bram_types.all;

entity ga is
  port (
    -- ga io
    clk    : in std_logic;
    config : in ga_config_t;
    go     : in boolean;
    done   : out boolean;
    pause  : in boolean;
    resume : in boolean;
    rng    : out std_logic_vector(31 downto 0); -- ga holds onto the global rng
    -- TODO: add outputs for generation fitness

    -- bram_manager (bm) io
    bm_command     : out bram_command_t := C_COPY_AND_MUTATE;
    bm_read_index  : out bram_index_t   := (others => '0');
    bm_write_index : out bram_index_t   := (others => '0');
    bm_go          : out boolean        := false;
    bm_done        : in boolean;

    -- tournament (tn) io
    tn_go            : out boolean := false;
    tn_done          : in boolean;
    tn_winner_counts : in winner_counts_array_t;

    -- fitness (ft) io
    ft_go   : out boolean;
    ft_done : in boolean
  );
end entity ga;

architecture ga_arch of ga is

  signal pause_queued : boolean := false;

  type state_t is (IDLE_S, PAUSED_S, INIT_RNG_S, INIT_NN_S);

begin

end architecture ga_arch;
