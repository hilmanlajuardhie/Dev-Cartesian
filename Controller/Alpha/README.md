# Alpha Controller

## Overview

Alpha is an early STM32CubeIDE/CubeMX firmware project for controlling a two-axis Cartesian robot. The project is located in `Alpha_test/` and targets an STM32F103C8T6 microcontroller.

Alpha is a basic motion prototype. It receives coordinate commands through USART1, converts the requested coordinate into motor steps, and generates GPIO step/direction signals for the horizontal and vertical axes.

## Project structure

```text
Controller/Alpha/
├── README.md
└── Alpha_test/
    ├── Alpha_test.ioc
    ├── .project
    ├── .cproject
    ├── Core/
    │   ├── Inc/
    │   ├── Src/
    │   └── Startup/
    ├── Drivers/
    │   ├── CMSIS/
    │   └── STM32F1xx_HAL_Driver/
    └── STM32F103C8TX_FLASH.ld
```

The `.ioc` file contains the CubeMX hardware configuration. The `.project` and `.cproject` files configure the project for STM32CubeIDE.

## Target hardware

| Item | Configuration |
|---|---|
| MCU | STM32F103C8T6 |
| CPU | ARM Cortex-M3 |
| System clock | 72 MHz |
| External oscillator | 8 MHz HSE |
| Flash | 64 KB |
| RAM | 20 KB |
| UART | USART1, 9600 baud, 8-N-1 |
| Debug | SWD |

The clock uses an 8 MHz external oscillator with PLL multiplier 9:

```text
8 MHz × 9 = 72 MHz
```

## Pin mapping

### USART1

| Signal | Pin |
|---|---|
| USART1 TX | PA9 |
| USART1 RX | PA10 |

### Motors

| Axis | Signal | Pin |
|---|---|---|
| Vertical motor 1 | Y1 step | PB0 |
| Vertical motor 1 | Y1 direction | PB1 |
| Vertical motor 2 | Y2 step | PB3 |
| Vertical motor 2 | Y2 direction | PB4 |
| Horizontal motor | X step | PB5 |
| Horizontal motor | X direction | PB6 |

The two vertical motors receive synchronized pulses and direction signals. This assumes that they drive opposite sides of the same gantry and are mechanically aligned.

## Coordinate system

Alpha uses a 10-by-20 logical grid:

```text
Rows:    A through J
Columns: 0 through 19
Steps:   10 steps per coordinate unit
```

The initial software position is assumed to be `A0`. For a target coordinate, Alpha calculates the difference from the current position, moves vertically first, and then moves horizontally.

Example:

```text
A0 → C5
```

produces:

```text
20 vertical steps
50 horizontal steps
```

## UART command format

Commands are text lines terminated by carriage return or newline:

```text
<row-letter> <column-number>
```

Examples:

```text
A 0
C 5
j 19
```

Both uppercase and lowercase row letters are accepted. Invalid rows and columns are ignored.

The receive buffer is small and Alpha does not currently provide a complete acknowledgment or error protocol.

## Firmware flow

After reset, the firmware:

1. initializes the STM32 HAL;
2. configures the 72 MHz clock;
3. initializes the motor GPIO outputs;
4. initializes USART1;
5. starts interrupt-driven UART reception;
6. waits for a complete command;
7. parses the command;
8. generates step pulses for the requested movement.

The main application is in `Alpha_test/Core/Src/main.c`. Interrupt handling is in `Core/Src/stm32f1xx_it.c`, and UART GPIO/peripheral setup is in `Core/Src/stm32f1xx_hal_msp.c`.

## Building Alpha

Alpha is a traditional STM32CubeIDE managed-build project, not a CMake project. Open the `Alpha_test` directory in STM32CubeIDE and select the Debug or Release configuration.

The required compiler is ARM GCC:

```text
arm-none-eabi-gcc
```

The generated build output normally includes an ELF file and may include HEX/BIN files depending on CubeIDE settings.

## Flashing Alpha

Use an ST-LINK debugger connected through SWD. Build the project first, then flash the generated ELF, HEX, or BIN file using STM32CubeIDE or STM32CubeProgrammer.

Before flashing, verify the board, oscillator, pin wiring, motor-driver logic voltage, and motor-driver enable state. A successful flash only confirms that the firmware was written; it does not confirm correct mechanical movement.

## Expected behavior after flashing

With the board powered and a serial terminal connected to USART1 at 9600 baud, Alpha should wait for coordinate commands. A valid command should cause motor pulses on the corresponding axis.

The software assumes that the mechanism starts at `A0`. If the carriage starts elsewhere or loses steps, the tracked position becomes inaccurate.

## Limitations

- No homing or limit-switch procedure.
- No encoder or position feedback.
- Open-loop positioning.
- Motor stepping uses a blocking software delay.
- No acceleration or deceleration profile.
- UART reception has limited buffering.
- No emergency-stop input in the application logic.
- No complete command acknowledgment protocol.

Alpha should be treated as a prototype/reference controller. For a more integrated controller, see `Controller/Base` and `Controller/Master`.
