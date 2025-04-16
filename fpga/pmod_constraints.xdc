# Minimal constraints file for KV260 UART design

# Map UART rx and tx to PMOD pins
# Using PMOD[0] for rx
# set_property PACKAGE_PIN H12      [get_ports "i_rx_serial"]
# set_property PACKAGE_PIN HDA11    [get_ports "i_rx_serial"]
#set_property PACKAGE_PIN this_should_fail    [get_ports "i_rx_serial"]
#set_property IOSTANDARD LVCMOS33  [get_ports "i_rx_serial"]

# Using PMOD[1] for tx
# set_property PACKAGE_PIN E10      [get_ports "o_tx_serial"]
#set_property PACKAGE_PIN HDA15    [get_ports "o_tx_serial"]
#set_property IOSTANDARD LVCMOS33  [get_ports "o_tx_serial"]

# Using PMOD[0] for rx (PMOD pin 1)
set_property PACKAGE_PIN H12      [get_ports "i_rx_serial"]
set_property IOSTANDARD LVCMOS33  [get_ports "i_rx_serial"]

# Using PMOD[1] for tx (PMOD pin 2)
set_property PACKAGE_PIN B10      [get_ports "o_tx_serial"]
set_property IOSTANDARD LVCMOS33  [get_ports "o_tx_serial"]

# Using PMOD[3] for indicator LED
set_property PACKAGE_PIN E10 [get_ports led_out]
set_property IOSTANDARD LVCMOS33 [get_ports led_out]
set_property SLEW SLOW [get_ports led_out]
set_property DRIVE 4 [get_ports led_out]

# False paths for asynchronous UART interface
set_false_path -from [get_ports i_rx_serial]
set_false_path -to [get_ports o_tx_serial]

set_property BITSTREAM.CONFIG.OVERTEMPSHUTDOWN ENABLE [current_design]

# Clock constraints for PS clocks
#create_clock -period 10.000 -name pl_clk0 -waveform {0.000 5.000} [get_pins zynq_ultra_ps_e_0/inst/PS8_i/PLCLK[0]]
#create_clock -period 10.000 -name pl_clk1 -waveform {0.000 5.000} [get_pins zynq_ultra_ps_e_0/inst/PS8_i/PLCLK[1]]
#create_clock -period 10.000 -name pl_clk0 [get_ports pl_clk0]
#create_clock -period 10.000 -name pl_clk1 [get_ports pl_clk1]

# # Clock constraints for PS clocks - use the hierarchy path to the PS clock pins
# create_clock -period 10.000 -name pl_clk0 -waveform {0.000 5.000} [get_pins */zynq_ultra_ps_e_0/inst/PS8_i/PLCLK[0]]

# # Create a generated clock on the top-level port that's driven by the PS clock
# create_generated_clock -name pl_clk0_0_clock -source [get_pins */zynq_ultra_ps_e_0/inst/PS8_i/PLCLK[0]] [get_ports pl_clk0_0]

# # Reset constraints - mark as asynchronous to prevent timing analysis on this path
# set_false_path -from [get_pins */zynq_ultra_ps_e_0/inst/PS8_i/PLRESETN[0]]
