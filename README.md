# Dev-Cartesian

Dev-Cartesian is a development workspace for a grid-based Cartesian robot. It contains a host-side simulator, several STM32 controller experiments, and CubeMX/CMake projects for STM32F103C8T6 boards.

## Repository layout

```text
Dev-Cartesian/
├── README.md
├── LICENSE
├── Simulation/                 Host-side coordinate simulator
├── Controller/
│   ├── Base/                   Early STM32 controller project
│   ├── Master/                 Integrated controller project
│   ├── Alpha/                  Earlier CubeIDE/CubeMX experiment
│   ├── S_1/                    Standalone simulator variant
│   ├── S_2/                    Standalone simulator variant
│   └── S_3/                    Standalone simulator variant
└── 3D_CAD/                    Mechanical/CAD assets
```

The `Base` and `Master` directories are CubeMX-generated embedded projects. Their `.ioc` files are the peripheral configuration sources, and their CMake files build the generated HAL/CMSIS projects with ARM GCC.

## Robot model

The software models a 10-by-20 Cartesian grid:

```text
Rows:    A through J, or 0 through 9 depending on the program
Columns: 0 through 19
Steps:   10 motor steps per grid-coordinate difference
```

The initial software position is normally row 0, column 0 (`A0`). A move is converted into row and column differences. The vertical Y axis moves first, followed by the horizontal X axis. For example, `A0` to `C5` produces 20 vertical steps and 50 horizontal steps.

Positioning is open-loop: the firmware assumes a known starting position and that motors do not lose steps. General homing, encoder feedback, and physical travel sensing are not yet implemented in the basic controllers.

## Simulation

`Simulation/Cartesian_Sim.c` is a terminal simulator for the coordinate and movement logic. It displays a 10-by-20 board and marks the current gantry position with `H`. It accepts commands such as `C 5` and `j 19`, checks bounds, prints simulated motor progress, updates the position, and exits when input ends. `Controller/S_1`, `S_2`, and `S_3` are similar standalone experiments with different input formats.

Build it with:

```bash
cc -std=c11 -Wall -Wextra -o Simulation/Cartesian_Sim Simulation/Cartesian_Sim.c
./Simulation/Cartesian_Sim
```

## Controller/Base

`Controller/Base` is an early STM32F103C8T6 controller. It contains USART1/USART2, USB configuration, GPIO step/direction outputs, interrupt-driven UART reception, and a 72 MHz clock derived from an 8 MHz external oscillator using PLL ×9.

Its intended pin map from `Base.ioc` is:

| Function | Pin |
|---|---|
| X direction / step | PB0 / PB1 |
| Y1 direction / step | PB4 / PB5 |
| Y2 direction / step | PB6 / PB7 |
| USART1 TX/RX | PA9 / PA10 |
| USART2 TX/RX | PA2 / PA3 |
| USB DM/DP | PA11 / PA12 |

The project is built through `CMakeLists.txt`, `CMakePresets.json`, and the ARM toolchain files under `Controller/Base/cmake`.

## Controller/Master

`Controller/Master` is the most integrated controller. It combines Cartesian motion, inter-controller UART routing, conveyor control, laser/object detection, and status messages. It uses USART1 and USART2 at 9600 baud, TIM2 as a microsecond timing source, USB FS, stepper-driver enable/direction/step outputs, a laser sensor, and conveyor outputs.

| Function | Signal | Pin |
|---|---|---|
| Laser/object input | `LASER_SENS` | PA8 |
| Upstream UART | USART1 TX/RX | PA9/PA10 |
| Downstream UART | USART2 TX/RX | PA2/PA3 |
| Conveyor | step/direction/enable | PB0/PB1/PB2 |
| Vertical axis | step/direction/enable | PB3/PB4/PB5 |
| Horizontal axis | step/direction/enable | PB6/PB7/PB8 |
| USB | USB DM/DP | PA11/PA12 |

The Master receives addressed text commands on USART1. It forwards commands for other controllers through USART2. Commands for itself trigger coordinate movement, a pick command, a wait, a return to `A0`, and a drop command/status sequence. The conveyor runs until the laser sensor changes state, then reports detection and disables the driver.

## Controller/Alpha

`Controller/Alpha/Alpha_test` is an older CubeIDE/CubeMX two-axis controller using USART1 and GPIO step/direction signals. Its pins and generated files differ from `Base` and `Master`, so those projects are not electrically interchangeable without checking the `.ioc`, generated headers, and wiring.

## Embedded architecture

```text
Application logic → STM32 HAL drivers → CMSIS device definitions → STM32F103 hardware
```

Important generated files include `Core/Src/main.c` for application logic, `stm32f1xx_hal_msp.c` for peripheral setup, `stm32f1xx_it.c` for interrupt handlers, startup assembly, the linker script, and the CMSIS/HAL driver directories. Keep custom code inside CubeMX `USER CODE` sections where possible.

## Build instructions

The embedded projects require CMake and ARM embedded GCC, including `arm-none-eabi-gcc`:

```bash
cd Controller/Master
cmake --preset Debug
cmake --build --preset Debug
```

Use the preset names listed by the selected project if they differ. STM32CubeIDE can also import `Base` or `Master`, build Debug/Release, and flash through ST-LINK over SWD.

## Current limitations

- No homing sequence, encoder feedback, or reliable position recovery.
- Motion and conveyor routines are blocking.
- UART data is shared between interrupt callbacks and the main loop without a robust command queue.
- No single documented packet format, checksum, acknowledgment, or error protocol exists across controllers.
- Emergency-stop and sensor-timeout handling are incomplete.
- No general acceleration/deceleration profile exists for stepper motors.
- Direction polarity, driver enable polarity, pulse timing, and wiring require hardware verification.
- Pin mappings differ between `Base`, `Master`, and `Alpha`.

## Recommended development order

1. Use the simulator as the behavior reference.
2. Establish one addressed UART protocol.
3. Add homing, limit switches, and maximum travel limits.
4. Replace blocking motion with a timer-driven state machine.
5. Add acceleration and deceleration.
6. Add UART ring buffers, checksums, acknowledgments, and error responses.
7. Add sensor timeouts and a hardware emergency-stop path.
8. Verify each controller's `.ioc`, generated headers, wiring, and driver polarity together.
9. Add host-side tests for parsing and coordinate movement.
10. Document motor drivers, power, mechanics, wiring, and CAD assets.

## License

See [LICENSE](LICENSE). The STM32 CMSIS and HAL directories also contain their own ST-provided license files.
