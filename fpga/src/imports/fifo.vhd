-- 7-Series FIFO18E1 (TODO: and/or FIFO36E1) wrapper.
-- Uses dual-clock mode with first-word fall-through.
-- See UG473 for information on 7-series built-in FIFO support.
-- See UG953 for information on the FIFO18E1/FIFO36E1 design elements.

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.math_real.log2;
use ieee.math_real.ceil;
use std.textio.all;

library unisim;
use unisim.vcomponents.all;

entity fifo is
  generic (
    -- Supported for now: 1-36. Depth is:
    --   If width =   1-4: 2048 TODO: this should be 4096 but need to figure out how to map bits.
    --   If width =   5-9: 2048
    --   If width = 10-18: 1024
    --   If width = 19-36: 512
    WIDTH       : natural := 18;
    FWFT        : boolean := true; -- First word fall through
    SYNCHRONOUS : boolean := false -- If synchronous, wr_clk must be the same as rd_clk
  );
  port (
    rst : in    boolean;

    -- Port A - Write only
    wr_clk  : in    std_logic;
    wr_en   : in    boolean;
    wr_data : in    std_logic_vector(WIDTH - 1 downto 0);
    full    : out   boolean;

    -- Port B - Read only
    rd_clk  : in    std_logic;
    rd_en   : in    boolean;
    rd_data : out   std_logic_vector(WIDTH - 1 downto 0) := (others => '0');
    empty   : out   boolean
  );
end entity fifo;

architecture rtl of fifo is

  -- Underlying FIFO18E1 width: 4, 9, 18, or 36.
  --  9-bit wide -> 1 8-bit byte  + 1 bits parity
  -- 18-bit wide -> 2 8-bit bytes + 2 bits parity
  -- 36-bit wide -> 4 8-bit bytes + 4 bits parity
  -- Can't just divide by 9 here, because that would produce 1, 2, 3, or 4 and we want 1, 2, or 4.
  -- This should probably just be a case statement...
  constant FIFO_BYTES : integer := 2 ** integer(ceil(log2(real((WIDTH - 1) / 9 + 1))));
  constant FIFO_WIDTH : integer := FIFO_BYTES * 9;

  -- An extra 1, 2, or 4 bits are stored in "parity" bits.
  -- Since ECC mode is not used, these are simply extra storage and not actually parity bits.
  constant DATA_WIDTH   : integer := minimum(WIDTH, FIFO_BYTES * 8);
  constant PARITY_WIDTH : integer := maximum(WIDTH - FIFO_BYTES * 8, 0); -- Ensure non-negative

  signal dip : std_logic_vector(3 downto 0)  := (others => '0');
  signal dop : std_logic_vector(3 downto 0);
  signal di  : std_logic_vector(31 downto 0) := (others => '0');
  signal do  : std_logic_vector(31 downto 0);

  signal empty_sl : std_logic;
  signal full_sl  : std_logic;

  -- Underlying FIFO mode, either 18 for WIDTH <= 18 or 18_36 for 18 < WIDTH <= 36
  function get_fifo_mode (fifo_width_in : natural) return string is
  begin
    if fifo_width_in > 18 then
      return "FIFO18_36";
    else
      return "FIFO18";
    end if;
  end function;

  function get_do_reg(sync : boolean) return integer is
  begin
    if sync then
      -- For synchronous FIFO, DO_REG must be set to 0 for flags and data to follow a
      -- standard synchronous FIFO operation. When DO_REG is set to 1, effectively a
      -- pipeline register is added to the output of the synchronous FIFO. Data then has a
      -- one clock cycle latency. However, the clock-to-out timing is improved.
      return 0;
    else
      -- Async fifos must set do_reg = 1
      return 1;
    end if;
  end function;

  function to_std_logic(b: boolean) return std_logic is
    variable sl : std_logic;
  begin
    sl := '1' when b else '0';
    return sl;
  end function;

begin

  -- TODO: Can these be compile-time?
  assert WIDTH <= 36                 report "FIFO WIDTH must be <=36"             severity error;
  assert not SYNCHRONOUS or not FWFT report "If SYNCHRONOUS, FWFT must be false." severity error;

  -- Split input into data and parity signals and join outputs into a single signal.
  -- Error correction is not enabled, so parity bits are simply extra memory.
  di(DATA_WIDTH - 1 downto 0) <= wr_data(DATA_WIDTH - 1 downto 0);

  wr_data_gen : if PARITY_WIDTH > 0 generate
    dip(PARITY_WIDTH - 1 downto 0) <= wr_data(WIDTH - 1 downto WIDTH - PARITY_WIDTH);
  end generate wr_data_gen;

  rd_data_gen : if PARITY_WIDTH > 0 generate
    rd_data <= dop(PARITY_WIDTH - 1 downto 0) & do(DATA_WIDTH - 1 downto 0);
  else generate
    rd_data <= do(DATA_WIDTH - 1 downto 0);
  end generate rd_data_gen;

  -- vsg_off instantiation_034 : Allow component instantiation here.
  -- vsg_off port_map_010      : Allow comments in this port map since we don't control the source.
  fifo18e1_inst : component fifo18e1
    generic map (
      -- Sets the almost empty and almost full thresholds
      ALMOST_EMPTY_OFFSET => x"0080",
      ALMOST_FULL_OFFSET  => x"0080",

      -- Initial value and set/reset value for output port
      INIT  => x"000000000",
      SRVAL => x"000000000",

      -- Sets data width to 4, 9, 18, or 36. If 36 FIFO_MODE must be "FIFO18_36".
      DATA_WIDTH => FIFO_WIDTH,
      FIFO_MODE  => get_fifo_mode(FIFO_WIDTH),

      -- Enable output register (1-0). Must be 1 if EN_SYN = false.
      DO_REG => get_do_reg(SYNCHRONOUS),

      -- Specifies FIFO as dual-clock (false) or Synchronous (true)
      EN_SYN => SYNCHRONOUS,

      -- Sets the FIFO FWFT to [true, false]. If true, EN_SYN must be false.
      FIRST_WORD_FALL_THROUGH => FWFT,

      -- Must be set to "7SERIES" for simulation behavior
      SIM_DEVICE => "7SERIES"
    )
    port map (
      -- Active-High (FIFO logic) asynchronous reset (EN_SYN=false),
      -- synchronous reset (EN_SYN=true). Must be held for a minimum of 5 WRCLK/RDCLK cycles.
      rst => to_std_logic(rst),

      -- Output register synchronous set/reset. DO_REG must be set to 1 if using this reset.
      rstreg => to_std_logic(rst),

      -- Write Control Signals: 1-bit (each) input: Write clock and enable input signals
      wrclk => wr_clk,              -- 1-bit input: Write clock
      wren  => to_std_logic(wr_en), -- 1-bit input: Write enable

      -- Write Data: 32-bit (each) input: Write input data
      di  => di,  -- 32-bit input: Data input
      dip => dip, -- 4-bit input: Parity input

      -- Rising edge read clock.
      rdclk => rd_clk,

      -- Active-High FIFO read enable.
      rden => to_std_logic(rd_en),

      -- Output register clock enable for pipelined synchronous FIFO.
      -- DO_REG must be set to 1 if using this enable.
      regce => '1',

      -- Read Data: 32-bit (each) output: Read output data
      do  => do,  -- 32-bit output: Data output
      dop => dop, -- 4-bit output: Parity data output

      -- Status: 1-bit (each) output: Flags and other FIFO status outputs
      almostempty => open,     -- 1-bit output: Almost empty flag
      almostfull  => open,     -- 1-bit output: Almost full flag
      empty       => empty_sl, -- 1-bit output: Empty flag
      full        => full_sl,  -- 1-bit output: Full flag
      rdcount     => open,     -- 12-bit output: Read count
      rderr       => open,     -- 1-bit output: Read error
      wrcount     => open,     -- 12-bit output: Write count
      wrerr       => open      -- 1-bit output: Write error
    );
  -- vsg_on

  empty <= empty_sl = '1';
  full  <= full_sl = '1';

end architecture rtl;