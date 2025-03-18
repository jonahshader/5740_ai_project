# Development Setup

## AMD Xilinx Vitis and Vivado 2024.2

This project uses Vivado and Vitis 2024.2.

### Download Setup File

Download [AMD Unified Installer for FPGAs & Adaptive SoCs 2024.2](https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/vitis.html). Ensure the version and OS is correct.

### Install Vitis/Vivado

1. Run the installer
2. Log in
3. On `Select Product to Install` screen select **Vitis** (this includes Vivado)
4. On `Vitis Unified Software Platform` screen, the following are needed: `Devices -> Install Devices for Kria SOMs and Starter Kits`, `Devices -> SoCs -> Zynq-7000`, `Devices -> SoCs -> Zynq-UltraScale+ MPSoC`, `Installation Options -> Install Cable Drivers`. You can also keep DocNav checked if you want offline documentation.

## VS Code

VS Code supports developing both VHDL and C++ so it's a good choice for this project.

To install on Windows download the installer [here](https://code.visualstudio.com/download).

Once VS Code is installed, open the repo folder.
VS Code should prompt you to install the recommended extensions, do so.
At the very least, editorconfig must be installed so that line endings, white space, etc
are setup properly.

### TerosHDL

You should be prompted to install the TerosHDL VS Code extension when first opening the workspace,
but if not search for and install TerosHDL in the Extensions Marketplace or search for @recommended.

TerosHDL provides syntax highlighting, go-to-definition and hover info, formatting, linting, testing, and more for VHDL development.
Installation instructions can be found [here](https://terostechnology.github.io/terosHDLdoc/docs/getting_started/installation).

The Python back-end for TerosHDL must be installed. For Linux the quick instructions are to run

```sh
pip3 install -r requirements.txt --break-system-packages
sudo apt install make
```

For Windows, follow the instructions from their website.

## Simulation

To compile the simulation, run the following in `bash` or `powershell`:

1. Create a folder for the build in the sim directory:

    ```bash
    mkdir build-debug
    cd build-debug
    ```

2. Configure the project with `CMAKE_BUILD_TYPE=Debug` or `CMAKE_BUILD_TYPE=Release`:

    ```bash
    cmake -DCMAKE_BUILD_TYPE=Debug ..
    ```

3. Build the project:

    ```bash
    cmake --build .
    ```

4. The executable will be placed at /Debug/sim.exe on Windows, TODO: where on Ubuntu?:

    ```bash
    ./Debug/sim
    ```
