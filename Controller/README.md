# Alpha Controller

## Overview

Alpha is a CMake-based STM32F103C8T6 controller project for the Cartesian robot. The current Alpha configuration is more advanced than the earlier two-axis prototype: it includes FreeRTOS, CMSIS-RTOS v2, USB Device DFU support, an independent watchdog, multiple stepper channels, home/limit inputs, and object-detection inputs.

The CubeMX configuration is `Alpha.ioc`. The generated application, middleware, and hardware files are kept directly under this folder.

## Project structure

```text
Controller/Alpha/
├── README.md
├── Alpha.ioc
├── CMakeLists.txt
├── CMakePresets.json
├── STM32F103xx_FLASH.ld
├── startup_stm32f103xb.s
├── Core/
│   ├── Inc/
│   └── Src/
├── Drivers/
│   ├── CMSIS/
│   └── STM32F1xx_HAL_Driver/
├── Middlewares/
│   ├── ST/STM32_USB_Device_Library/
│   └── Third_Party/FreeRTOS/
└── USB_DEVICE/
```

`Alpha.ioc` is the CubeMX source configuration. `CMakeLists.txt` and `CMakePresets.json` define the build. `Core` contains the application and generated startup support, while `Drivers` and `Middlewares` contain the STM32 HAL, CMSIS, USB, and FreeRTOS code.

## Target hardware

| Item | Configuration |
|---|---|
| MCU | STM32F103C8T6 |
| CPU | ARM Cortex-M3 |
| System clock | 72 MHz |
| External oscillator | 8 MHz HSE |
| Flash | 64 KB |
| RAM | 20 KB |
| UART | USART1, 115200 baud, 8-N-1 |
| USB | USB FS, DFU class |
| RTOS | FreeRTOS through CMSIS-RTOS v2 |
| Watchdog | Independent watchdog, IWDG |
| Debug | SWD |

The system clock uses an 8 MHz external oscillator and PLL multiplier 9:

```text
8 MHz × 9 = 72 MHz
```

The USB clock is derived from the PLL at 48 MHz. TIM1 is configured as the HAL/RTOS time base, and the independent watchdog is enabled to reset the MCU if the application stops servicing it.

## Pin mapping

### UART and USB

| Function | Pin |
|---|---|
| USART1 TX | PA9 |
| USART1 RX | PA10 |
| USB DM | PA11 |
| USB DP | PA12 |

USART1 is configured for 115200 baud, 8 data bits, no parity, and one stop bit. USB is configured for Device Firmware Upgrade (DFU), which may be used for firmware update workflows if the USB DFU application is completed and the board wiring supports it.

### Motor and actuator outputs

| Device | Step | Direction | Enable |
|---|---|---|---|
| X axis | PA0 | PA1 | PA2 |
| Y axis | PA3 | PA4 | PA5 |
| Conveyor | PA6 | PA7 | PB0 |
| Gripper | PB1 | PB10 | PB11 |

The exact active level for each enable and direction signal must be verified against the connected motor drivers. The physical wiring must match `Alpha.ioc` and the generated definitions in `Core/Inc/main.h`.

### Sensors and limits

| Input | Pin |
|---|---|
| X home | PB12 |
| X maximum | PB13 |
| Y home | PB14 |
| Y maximum | PB15 |
| Conveyor detection | PB6 |
| Gripper detection | PB7 |

These inputs are configured in CubeMX, but their complete use in the application must be verified in `Core/Src/main.c` and related tasks.

## Software architecture

```text
Application tasks
        ↓
CMSIS-RTOS v2 API
        ↓
FreeRTOS kernel
        ↓
STM32 HAL / CMSIS
        ↓
STM32F103 hardware
```

The application initializes the HAL and peripherals, initializes the RTOS kernel, creates `defaultTask`, and starts the scheduler. The default task implementation is in `Core/Src/freertos.c`.

The main application entry point is `Core/Src/main.c`. Hardware initialization is generated in `Core/Src/stm32f1xx_hal_msp.c`, interrupt handling is in `Core/Src/stm32f1xx_it.c`, and the USB DFU application is under `USB_DEVICE/`.

## Current runtime behavior

After reset, Alpha is expected to:

1. initialize the STM32 HAL;
2. configure the 72 MHz system clock and 48 MHz USB clock;
3. initialize GPIO, the watchdog, and USART1;
4. initialize the USB Device DFU stack;
5. initialize the CMSIS-RTOS/FreeRTOS kernel;
6. create the default RTOS task;
7. start the scheduler;
8. run the configured application task(s).

The current source contains the generated RTOS structure and hardware configuration. Motor motion, homing, conveyor handling, gripper behavior, and sensor responses must be implemented or confirmed in the application tasks before expecting a complete robot cycle.

## Coordinate and motion model

Alpha is intended to control the Cartesian X and Y axes, with additional conveyor and gripper actuators. The workspace generally uses a logical grid of rows and columns, but the current Alpha application should be treated as the authoritative source for the active command format and motion parameters.

Do not assume that Alpha uses the same UART protocol, pin mapping, baud rate, or steps-per-unit values as Base or Master. These projects are separate firmware targets.

## Building Alpha

Alpha uses CMake and Ninja with the ARM embedded GCC toolchain. Required tools include:

```text
cmake
ninja
arm-none-eabi-gcc
arm-none-eabi-g++
arm-none-eabi-objcopy
arm-none-eabi-size
```

Build a Debug image:

```bash
cd Controller/Alpha
cmake --preset Debug
cmake --build --preset Debug
```

Build a Release image:

```bash
cd Controller/Alpha
cmake --preset Release
cmake --build --preset Release
```

The presets use the toolchain file at:

```text
Controller/Alpha/cmake/gcc-arm-none-eabi.cmake
```

The linker script is `STM32F103xx_FLASH.ld`. Build output is normally placed under `Controller/Alpha/build/Debug` or `Controller/Alpha/build/Release`.

## Flashing and USB DFU

For SWD development, connect an ST-LINK debugger and flash the generated ELF, HEX, or BIN file using STM32CubeProgrammer, OpenOCD, or a compatible tool.

Example:

```bash
STM32_Programmer_CLI -c port=SWD \
    -w build/Debug/Alpha.elf \
    -rst
```

The USB Device configuration uses the DFU class. DFU support requires the USB data lines, USB clock, descriptors, DFU interface implementation, and host-side DFU procedure to be correct. SWD should remain the primary recovery/debug method until USB DFU has been validated.

## What to expect after flashing

A successful flash only confirms that the firmware image was written to the MCU. After reset, the expected observable behavior is:

- the MCU starts the RTOS scheduler;
- the watchdog is active;
- USART1 is available at 115200 baud;
- USB may enumerate as a DFU device if the DFU implementation is complete;
- configured GPIO outputs reach their startup states;
- application tasks run according to their RTOS priorities.

Actual motor movement or conveyor/gripper operation will occur only if the corresponding application tasks and command handling are implemented and the hardware is connected correctly.

## Hardware safety checklist

Before enabling motors:

1. Verify the board is an STM32F103C8T6 or compatible STM32F103xB device.
2. Verify the external oscillator is 8 MHz.
3. Confirm the motor-driver logic voltage is compatible with 3.3 V GPIO.
4. Confirm STM32 ground and driver logic ground are connected.
5. Confirm all step, direction, and enable pins against `Alpha.ioc`.
6. Test enable polarity with motor power disconnected.
7. Verify X/Y home and maximum switch wiring.
8. Provide a physical emergency-stop method.
9. Test one output and one axis at low speed first.
10. Confirm that the watchdog cannot create an unsafe repeated motion condition.

## Limitations and development work

- The RTOS task structure is present, but complete machine behavior still requires application-task implementation.
- Motor control, homing, and sensor handling must be validated on the real hardware.
- There is no documented complete command protocol in this folder yet.
- Enable and direction polarities depend on the connected drivers.
- USB DFU requires end-to-end validation.
- Watchdog recovery behavior must be tested for safe actuator states.
- Acceleration, deceleration, travel limits, and emergency-stop behavior must be designed before production use.

## Relationship to other controllers

Alpha, Base, and Master all target STM32F103C8T6 devices, but they are separate firmware projects with different peripherals, pins, timing, and communication settings.

```text
Alpha   FreeRTOS + USB DFU + multiple actuators and sensors
Base    Basic STM32 controller foundation
Master  Integrated motion, conveyor, laser, and UART workflow
```

Never flash an image from one project onto hardware wired for another project without checking the project-specific `.ioc`, generated headers, linker configuration, and wiring.
