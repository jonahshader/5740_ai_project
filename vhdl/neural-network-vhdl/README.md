# Neural Network VHDL Project

## Overview
This project implements a neural network using VHDL. It includes components for neurons, activation functions, and utility functions, allowing for the creation of a configurable neural network architecture.

## Project Structure
```
neural-network-vhdl
├── src
│   ├── components.vhdl        # Reusable components for the neural network
│   ├── NN.vhdl                # Top-level entity for the neural network
│   ├── neuron.vhdl            # Definition of the Generic_Neuron entity
│   ├── activation.vhdl        # ReLU activation function
│   ├── register.vhdl          # Generic register for storing values
│   ├── Mux2to1.vhdl           # 2-to-1 multiplexer
│   └── testbench
│       └── NN_tb.vhdl         # Testbench for the NN.vhdl file
├── scripts
│   └── simulation.do          # Script for running simulations
└── README.md                  # Documentation for the project
```

## File Descriptions
- **src/components.vhdl**: Contains reusable components such as multiplexers and utility functions that can be utilized throughout the neural network.
  
- **src/NN.vhdl**: The main entry point for the neural network. This file instantiates input neurons, hidden neurons (with support for multiple hidden layers), and output neurons using the `Generic_Neuron` entity defined in `neuron.vhdl`.

- **src/neuron.vhdl**: Defines the `Generic_Neuron` entity, which represents a single neuron. It includes ports for clock, reset, weights, inputs, bias, output, and derivative.

- **src/activation.vhdl**: Implements the ReLU activation function, which applies activation logic to the summed input of the neurons.

- **src/register.vhdl**: Defines a generic register that can be used for storing intermediate results within the neural network.

- **src/Mux2to1.vhdl**: Implements a 2-to-1 multiplexer, useful for selecting between two inputs in various parts of the neural network.

- **src/testbench/NN_tb.vhdl**: A testbench designed to simulate and verify the functionality of the neural network defined in `NN.vhdl`.

- **scripts/simulation.do**: A script for executing simulations, likely intended for use with a VHDL simulator.

## Setup Instructions
1. Ensure you have a VHDL simulator installed.
2. Clone the repository or download the project files.
3. Navigate to the `scripts` directory and run the `simulation.do` script to execute the simulations.

## Usage
- Modify the parameters in `NN.vhdl` to configure the number of input neurons, hidden layers, and output neurons as needed for your specific application.
- Use the provided testbench in `NN_tb.vhdl` to validate the functionality of your neural network design.

## License
This project is open-source and can be freely used and modified. However, users are responsible for any consequences arising from the use of this code.