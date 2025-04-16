library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;
use ieee.numeric_std.all;

entity neuron_tb is

end entity neuron_tb;

architecture testbench of neuron_tb is
  -- Constants for the configuration
  constant CLK_PERIOD : time := 10 ns;
  constant NUM_INPUTS : integer := 4;
  constant DATA_WIDTH : integer := 8;

  component neuron is
    generic (
      NUM_INPUTS : integer;
      DATA_WIDTH : integer
    );
    port (
      clk        : in std_logic;
      rst        : in std_logic;
      weights    : in std_logic_vector(NUM_INPUTS * DATA_WIDTH - 1 downto 0);
      inputs     : in std_logic_vector(NUM_INPUTS * DATA_WIDTH - 1 downto 0);
      bias       : in std_logic_vector(DATA_WIDTH - 1 downto 0);
      output     : out std_logic_vector(DATA_WIDTH - 1 downto 0)
    );
  end component;

  -- Test signals
  signal clk        : std_logic := '0';
  signal rst        : std_logic := '1';
  signal weights    : std_logic_vector(NUM_INPUTS * DATA_WIDTH - 1 downto 0) := (others => '0');
  signal inputs     : std_logic_vector(NUM_INPUTS * DATA_WIDTH - 1 downto 0) := (others => '0');
  signal bias       : std_logic_vector(DATA_WIDTH - 1 downto 0) := (others => '0');
  signal output     : std_logic_vector(DATA_WIDTH - 1 downto 0);
  signal test_done  : boolean := false;

begin

  dut: neuron
    generic map (
      NUM_INPUTS => NUM_INPUTS,
      DATA_WIDTH => DATA_WIDTH
    )
    port map (
      clk     => clk,
      rst     => rst,
      weights => weights,
      inputs  => inputs,
      bias    => bias,
      output  => output
    );

  clk_process: process
  begin
    while not test_done loop
      clk <= '0';
      wait for CLK_PERIOD/2;
      clk <= '1';
      wait for CLK_PERIOD/2;
    end loop;
    wait;
  end process;

  stimulus: process
  begin
    -- Initialize and reset
    rst <= '1';
    wait for CLK_PERIOD * 2;
    rst <= '0';
    wait for CLK_PERIOD;

    -- Test case 1: All inputs and weights are 1, bias is 0
    -- Expected: 4 (if using small values with no overflow)
    for i in 0 to NUM_INPUTS-1 loop
      weights((i+1)*DATA_WIDTH-1 downto i*DATA_WIDTH) <= std_logic_vector(to_signed(1, DATA_WIDTH));
      inputs((i+1)*DATA_WIDTH-1 downto i*DATA_WIDTH) <= std_logic_vector(to_signed(1, DATA_WIDTH));
    end loop;
    bias <= std_logic_vector(to_signed(0, DATA_WIDTH));

    wait for CLK_PERIOD * 3; -- Wait a few clock cycles for processing

    -- Print results
    report "Test Case 1: Output = " & integer'image(to_integer(signed(output)));

    -- Test case 2: Alternating positive and negative values
    for i in 0 to NUM_INPUTS-1 loop
      if i mod 2 = 0 then
        weights((i+1)*DATA_WIDTH-1 downto i*DATA_WIDTH) <= std_logic_vector(to_signed(2, DATA_WIDTH));
        inputs((i+1)*DATA_WIDTH-1 downto i*DATA_WIDTH) <= std_logic_vector(to_signed(1, DATA_WIDTH));
      else
        weights((i+1)*DATA_WIDTH-1 downto i*DATA_WIDTH) <= std_logic_vector(to_signed(-1, DATA_WIDTH));
        inputs((i+1)*DATA_WIDTH-1 downto i*DATA_WIDTH) <= std_logic_vector(to_signed(2, DATA_WIDTH));
      end if;
    end loop;
    bias <= std_logic_vector(to_signed(1, DATA_WIDTH));

    wait for CLK_PERIOD * 3;

    -- Print results
    report "Test Case 2: Output = " & integer'image(to_integer(signed(output)));

    -- Test case 3: Test ReLU with negative result
    for i in 0 to NUM_INPUTS-1 loop
      weights((i+1)*DATA_WIDTH-1 downto i*DATA_WIDTH) <= std_logic_vector(to_signed(-2, DATA_WIDTH));
      inputs((i+1)*DATA_WIDTH-1 downto i*DATA_WIDTH) <= std_logic_vector(to_signed(1, DATA_WIDTH));
    end loop;
    bias <= std_logic_vector(to_signed(0, DATA_WIDTH));

    wait for CLK_PERIOD * 3;

    -- Print results (should be 0 due to ReLU)
    report "Test Case 3: Output = " & integer'image(to_integer(signed(output)));

    -- End simulation
    wait for CLK_PERIOD * 2;
    test_done <= true;
    wait;
  end process;

end architecture testbench;
