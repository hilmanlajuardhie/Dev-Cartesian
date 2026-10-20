# Cartesian Robot Simulator

## Purpose

The Simulation directory contains host-side C programs that model the Cartesian robot's coordinate movement without requiring an STM32 board, motor drivers, or mechanical hardware.

The main simulator is `Simulation/Cartesian_Sim.c`. It displays a 10-by-20 table, tracks the current carriage position, validates target coordinates, and simulates vertical and horizontal motor movement.

## Coordinate model

```text
Rows:    A through J
Columns: 0 through 19
Steps:   10 simulated steps per coordinate difference
```

The initial position is `A0`. The simulator moves the vertical axis first and then the horizontal axis. Moving from `A0` to `C5` simulates 20 vertical steps and 50 horizontal steps.

## Build and run

From the repository root:

```bash
cc -std=c11 -Wall -Wextra \
   -o Simulation/Cartesian_Sim \
   Simulation/Cartesian_Sim.c

./Simulation/Cartesian_Sim
```

Commands use this format:

```text
<row-letter> <column-number>
```

Examples:

```text
C 5
j 19
```

## Scope

The simulator is useful for checking coordinate bounds, position tracking, difference calculations, movement order, and the relationship between coordinate units and simulated steps.

It does not prove STM32 firmware correctness, GPIO wiring, driver polarity, real step timing, acceleration, homing, limit-switch behavior, mechanical accuracy, or emergency-stop safety.

`Controller/S_1`, `Controller/S_2`, and `Controller/S_3` contain earlier standalone desktop C experiments. They are not STM32 firmware. For the complete workspace description, see [../README.md](../README.md).
