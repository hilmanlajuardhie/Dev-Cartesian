# Dev-Cartesian

Dev-Cartesian is a development workspace for a grid-based Cartesian robot. It contains a host-side simulator and three STM32F103C8T6 firmware projects: `Alpha`, `Base`, and `Master`. The current controller projects use STM32CubeMX-generated code and CMake; Alpha additionally includes FreeRTOS and USB DFU support.

## Repository layout

```text
Dev-Cartesian/
├── README.md
├── LICENSE
├── Simulation/                 Host-side coordinate simulator
├── Controller/
│   ├── Base/                   Early STM32 controller project
│   ├── Master/                 Integrated controller project
│   ├── Alpha/                  FreeRTOS/USB-enabled controller
│   ├── S_1/                    Standalone simulator variant
│   ├── S_2/                    Standalone simulator variant
│   └── S_3/                    Standalone simulator variant
├── Controller/Test/            Current test area, if populated
├── Controller/Base.zip         Archived Base project package
└── 3D_CAD/                    Mechanical/CAD assets
```

The `Alpha`, `Base`, and `Master` directories are CubeMX-generated embedded projects. Their `.ioc` files are the peripheral configuration sources, and their CMake files build the generated HAL/CMSIS projects with ARM GCC. The project-specific README files provide details for Alpha and Base.

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

`Controller/Alpha` is now a CMake-based STM32F103C8T6 project with a larger machine-control configuration than the earlier Alpha prototype. Its CubeMX configuration enables:

- FreeRTOS through CMSIS-RTOS v2;
- USB Device DFU support;
- USART1 at 115200 baud;
- independent X, Y, conveyor, and gripper stepper signals;
- X/Y home and maximum limit inputs;
- conveyor and gripper detection inputs;
- an independent watchdog;
- TIM1 as the RTOS time base.

The main motion and RTOS application files are under `Controller/Alpha/Core`. FreeRTOS sources are under `Controller/Alpha/Middlewares/Third_Party/FreeRTOS`, and USB DFU sources are under `Controller/Alpha/USB_DEVICE` and `Controller/Alpha/Middlewares/ST/STM32_USB_Device_Library`.

The main Alpha signals are:

| Function | Pin |
|---|---|
| X step/direction/enable | PA0/PA1/PA2 |
| Y step/direction/enable | PA3/PA4/PA5 |
| Conveyor step/direction/enable | PA6/PA7/PB0 |
| Gripper step/direction/enable | PB1/PB10/PB11 |
| X home / maximum | PB12/PB13 |
| Y home / maximum | PB14/PB15 |
| Conveyor/gripper detection | PB6/PB7 |
| USART1 TX/RX | PA9/PA10 |
| USB DM/DP | PA11/PA12 |

Alpha is not electrically interchangeable with Base or Master. Always verify its own `Alpha.ioc`, generated `main.h`, and wiring.

## Embedded architecture

```text
Application logic → STM32 HAL drivers → CMSIS device definitions → STM32F103 hardware
```

Important generated files include `Core/Src/main.c` for application logic, `stm32f1xx_hal_msp.c` for peripheral setup, `stm32f1xx_it.c` for interrupt handlers, startup assembly, the linker script, and the CMSIS/HAL driver directories. Keep custom code inside CubeMX `USER CODE` sections where possible.

## Build instructions

The embedded projects require CMake, Ninja, and ARM embedded GCC, including `arm-none-eabi-gcc`:

```bash
cd Controller/Master
cmake --preset Debug
cmake --build --preset Debug
```

Alpha can be built in the same way:

```bash
cd Controller/Alpha
cmake --preset Debug
cmake --build --preset Debug
```

Use the preset names listed by the selected project if they differ. STM32CubeIDE can also import the CubeMX projects, build Debug/Release, and flash through ST-LINK over SWD.

## Current limitations

- Homing inputs are configured in Alpha, but a complete validated homing and recovery workflow must still be confirmed in the application and on hardware.
- Basic open-loop controllers have no encoder feedback or reliable position recovery.
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
