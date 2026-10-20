# Dev-Cartesian Workspace Guide

## 1. What is this workspace?

Dev-Cartesian is a development workspace for a Cartesian robot controlled by STM32F103C8T6 microcontrollers.

The robot is modeled as a rectangular grid. A carriage can move horizontally and vertically to a requested coordinate. The workspace contains both computer-side simulations and embedded STM32 firmware projects so that the movement concept can be tested before it is connected to real motors.

The workspace includes:

- a terminal simulator for the Cartesian table;
- several small standalone simulation programs;
- an early STM32 controller called `Alpha`;
- an STM32 foundation project called `Base`;
- a more integrated controller called `Master`;
- STM32 HAL, CMSIS, startup, and linker files;
- CubeMX `.ioc` hardware configuration files;
- CMake build files for the newer controller projects;
- CAD assets for the mechanical system.

This is a development repository, not yet a complete production machine controller. The firmware currently performs open-loop movement and depends on the actual wiring, motor drivers, mechanical limits, and sensors being correctly installed.

## 2. Workspace structure

```text
Dev-Cartesian/
├── README.md                 General project summary
├── README.mc                 This detailed workspace guide
├── LICENSE                   Repository license
├── Simulation/
│   ├── Cartesian_Sim.c       Main host-side simulator
│   ├── Cartesian_Sim         Compiled simulator executable
│   └── README.md             Simulator notes
├── Controller/
│   ├── Alpha/
│   │   └── Alpha_test/       Earlier CubeIDE STM32 project
│   ├── Base/                 STM32 foundation project
│   ├── Master/               Integrated STM32 controller
│   ├── S_1/                  Standalone simulation variant
│   ├── S_2/                  Standalone simulation variant
│   └── S_3/                  Standalone simulation variant
├── 3D_CAD/                   Mechanical and CAD assets
└── .vscode/                  Workspace editor tasks
```

## 3. How the workspace works

The development flow is intended to be:

```text
Test movement in the simulator
          ↓
Develop and compile STM32 firmware
          ↓
Flash firmware to an STM32 board
          ↓
Send coordinate or machine commands
          ↓
The controller drives motors, sensors, and conveyor hardware
```

The simulator and embedded firmware use the same basic movement idea:

1. Maintain a current logical position.
2. Receive a target position.
3. Check whether the target is inside the allowed grid.
4. Calculate the row and column differences.
5. Convert coordinate differences into motor steps.
6. Move the vertical axis.
7. Move the horizontal axis.
8. Update the software position.

The normal logical grid is:

```text
Rows:    A through J, or 0 through 9
Columns: 0 through 19
Steps:   10 steps for each coordinate difference
```

For example, a move from `A0` to `C5` means:

```text
Row difference:    2 × 10 = 20 vertical steps
Column difference: 5 × 10 = 50 horizontal steps
```

The vertical axis is generally represented by two synchronized motors. The horizontal axis is represented by one motor in the basic designs.

## 4. Simulation layer

The main simulator is:

```text
Simulation/Cartesian_Sim.c
```

It runs on the development computer and does not require an STM32, motor driver, or ST-LINK.

The simulator displays a 10-by-20 table. The current carriage position is shown using `H`. It accepts coordinate input, validates the target, prints simulated motor progress, and updates the displayed position.

Build and run it with:

```bash
cc -std=c11 -Wall -Wextra \
   -o Simulation/Cartesian_Sim \
   Simulation/Cartesian_Sim.c

./Simulation/Cartesian_Sim
```

Typical input is:

```text
C 5
j 19
```

The simulator is useful for checking coordinate limits and movement behavior, but it does not prove that the physical motor wiring, driver polarity, pulse timing, mechanics, or sensors are correct.

The programs in `Controller/S_1`, `Controller/S_2`, and `Controller/S_3` are earlier standalone variants of this simulation idea. They are not STM32 firmware and do not need the ARM embedded compiler or flashing tools.

## 5. Alpha controller

The Alpha project is located at:

```text
Controller/Alpha/Alpha_test/
```

Alpha is a traditional STM32CubeIDE project for the STM32F103C8T6. It contains:

- `Alpha_test.ioc` CubeMX configuration;
- STM32 HAL and CMSIS drivers;
- generated GPIO and USART initialization;
- startup assembly;
- linker script;
- `.project` and `.cproject` CubeIDE metadata;
- application logic in `Core/Src/main.c`.

Alpha receives coordinate commands over USART1 at 9600 baud and generates GPIO step/direction pulses for the X and Y motors.

Its principal pins are:

| Function | Pin |
|---|---|
| Y1 step | PB0 |
| Y1 direction | PB1 |
| Y2 step | PB3 |
| Y2 direction | PB4 |
| X step | PB5 |
| X direction | PB6 |
| USART1 TX | PA9 |
| USART1 RX | PA10 |

Alpha is mainly an early two-axis prototype. It is useful for understanding the original movement logic, but it is not currently part of the CMake build flow used by Base and Master.

## 6. Base controller

The Base project is located at:

```text
Controller/Base/
```

Base is a newer STM32F103C8T6 project with CMake support. It contains:

- `Base.ioc` CubeMX configuration;
- `CMakeLists.txt`;
- `CMakePresets.json`;
- ARM GCC toolchain files;
- STM32 HAL/CMSIS source;
- linker script;
- startup assembly;
- application source under `Core`.

The intended Base motor mapping is:

| Function | Pin |
|---|---|
| X direction | PB0 |
| X step | PB1 |
| Y1 direction | PB4 |
| Y1 step | PB5 |
| Y2 direction | PB6 |
| Y2 step | PB7 |
| USART1 TX/RX | PA9/PA10 |
| USART2 TX/RX | PA2/PA3 |
| USB DM/DP | PA11/PA12 |

Base is best treated as a controller foundation or intermediate prototype. The `.ioc` file, generated headers, source code, and physical wiring must be checked together because the controller projects use different pin assignments.

## 7. Master controller

The Master project is located at:

```text
Controller/Master/
```

Master is the most integrated controller in the workspace. It adds machine-level behavior on top of basic Cartesian movement.

Its features include:

- horizontal and vertical stepper control;
- motor enable signals;
- conveyor control;
- laser/object sensing;
- USART1 communication with an upstream device;
- USART2 communication with downstream controllers;
- status messages;
- TIM2-based microsecond timing;
- USB FS configuration;
- pick-and-drop workflow logic.

The intended Master pin map is:

| Function | Pin |
|---|---|
| Laser/object sensor | PA8 |
| USART1 TX/RX | PA9/PA10 |
| USART2 TX/RX | PA2/PA3 |
| Conveyor step/direction/enable | PB0/PB1/PB2 |
| Vertical step/direction/enable | PB3/PB4/PB5 |
| Horizontal step/direction/enable | PB6/PB7/PB8 |
| USB DM/DP | PA11/PA12 |

The intended Master process is:

```text
Receive command on USART1
          ↓
Check destination MCU ID
          ↓
Forward command through USART2
or
          ↓
Move to requested coordinate
          ↓
Perform pick operation
          ↓
Return to A0
          ↓
Perform drop operation
          ↓
Send status messages
```

The conveyor routine drives the conveyor until the laser sensor detects an object. It then stops the conveyor and reports the event.

## 8. Expected behavior after flashing

Flashing an STM32 project does not automatically make the complete robot operate. The expected result depends on which project is flashed and whether the hardware is correctly connected.

### After flashing Alpha

After reset, Alpha should:

1. initialize the STM32 HAL;
2. configure the 72 MHz clock;
3. initialize GPIO outputs;
4. initialize USART1 at 9600 baud;
5. start interrupt-driven UART reception;
6. wait for a coordinate command.

The expected command format is approximately:

```text
A 0
C 5
J 19
```

When a valid command is received, the controller generates step pulses. Alpha does not provide a comprehensive acknowledgment or completion protocol.

### After flashing Base

Base should initialize its configured STM32 peripherals and wait for its application commands. Its exact runtime behavior depends on the current `Core/Src/main.c` implementation and the matching `.ioc` configuration.

Base should be considered a platform/prototype firmware until its motor outputs and command protocol have been verified on hardware.

### After flashing Master

Master should initialize:

- system clock;
- GPIO;
- TIM2;
- USART1;
- USART2;
- USB support;
- UART reception.

It should then wait for an addressed command. Depending on the destination ID, it either forwards the command to another controller or performs its own movement and pick/drop sequence.

Status messages are expected through the configured UART output. The exact message sequence depends on the command and sensor state.

## 9. What must be true before flashing hardware

Before flashing any controller, verify:

1. The board is actually an STM32F103C8T6 or compatible STM32F103xB device.
2. The external oscillator matches the configured 8 MHz HSE.
3. Motor drivers share a correct logic ground with the STM32.
4. Step, direction, and enable pins match the selected project.
5. Motor driver logic voltage is compatible with STM32 3.3 V signals.
6. Motor power is supplied separately and safely.
7. Driver enable polarity is understood.
8. Direction polarity is tested at low speed.
9. Mechanical travel limits are installed or the axis is physically protected.
10. An emergency-stop method is available.

Never assume that the Alpha, Base, and Master pin maps are interchangeable.

## 10. Building the STM32 projects

Base and Master use CMake presets. The required tools are:

```text
cmake
ninja
arm-none-eabi-gcc
arm-none-eabi-g++
arm-none-eabi-objcopy
arm-none-eabi-size
```

Build Master in Debug mode with:

```bash
cd Controller/Master
cmake --preset Debug
cmake --build --preset Debug
```

Build Base with:

```bash
cd Controller/Base
cmake --preset Debug
cmake --build --preset Debug
```

The expected output is an ELF firmware image and associated build files under the project `build/Debug` directory.

Alpha is configured as a CubeIDE managed-build project. It can be built through STM32CubeIDE, or it can be converted later to the same CMake structure used by Base and Master.

## 11. Flashing and debugging

The firmware requires an SWD programmer/debugger, normally ST-LINK. A flashing utility must also be installed, such as STM32CubeProgrammer, OpenOCD, or `st-flash`.

A typical STM32CubeProgrammer command is:

```bash
STM32_Programmer_CLI -c port=SWD -w build/Debug/Master.elf -rst
```

The exact ELF path depends on the selected project and build configuration.

After flashing, use a serial terminal at the configured baud rate to send commands and observe status output. The current workspace does not yet contain a complete root-level VS Code flash and debug configuration.

## 12. Important limitations

The current firmware is open-loop and prototype-level. Important limitations include:

- no general homing procedure;
- no encoder or position feedback;
- possible lost-step position errors;
- blocking motor and conveyor routines;
- limited UART buffering;
- incomplete command queue handling;
- no universal checksum or acknowledgment protocol;
- incomplete sensor timeout handling;
- no complete emergency-stop implementation;
- no acceleration/deceleration profile;
- direction and enable polarity depend on the hardware;
- different projects use different GPIO pin mappings.

These limitations matter especially after flashing. A successful flash only proves that code was written to the microcontroller. It does not prove that the robot is safe or mechanically calibrated.

## 13. Recommended development workflow

1. Test coordinate behavior with `Simulation/Cartesian_Sim.c`.
2. Select one authoritative controller project, preferably Master for integrated behavior.
3. Verify its `.ioc`, generated headers, source code, and wiring.
4. Install and verify the ARM GCC compiler.
5. Build Debug firmware.
6. Flash with the motor power disconnected or drivers disabled initially.
7. Test UART communication without motion.
8. Test one axis at low speed.
9. Verify direction and enable polarity.
10. Add homing and limit-switch protection.
11. Test the second axis.
12. Test conveyor and sensor behavior with timeouts.
13. Add acceleration, command acknowledgments, and emergency-stop handling.

## 14. Project maturity

The folders represent different development stages:

```text
S_1/S_2/S_3  →  Simulation  →  Alpha  →  Base  →  Master
```

This is a useful progression, but the projects are not yet one unified firmware system. Source logic, protocols, and GPIO assignments should eventually be consolidated or documented so that a change in one project does not silently disagree with another.

## 15. License

See [LICENSE](LICENSE). The STM32 CMSIS and HAL directories also contain their own ST-provided license files.
