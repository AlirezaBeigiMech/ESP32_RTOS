# Ella Says "Foo-Bar"

## Project Overview

This project implements a program for the ESP32 using C++ and FreeRTOS, designed to solve the "Ella Says 'Foo-Bar'" task. The program reads a positive integer from the serial input, counts down from that integer, and outputs specific messages based on the current count. This project demonstrates the use of multi-core processing and task synchronization with FreeRTOS.

## Features

1. **Serial Input Handling:**
   - The program prompts for a positive integer `N` through the serial input. The input is expected in character form (e.g., `'123456'`). The input `N` must be greater than zero.

2. **Countdown Functionality:**
   - The program counts down from `M = N` once per second. Based on the current count `M`, the program performs the following:
     - If `M` is even, a task running on Core 0 outputs the string `"Foo"` followed by the current count to the serial monitor.
     - If `M` is odd, a task running on Core 1 outputs the string `"Bar"` followed by the current count to the serial monitor.
     - If `M` is a prime number, a task running on Core 0 additionally outputs the string `"Prime"`.

3. **Example Output:**
   - For `N = 6`, the expected sequence output is:
     ```
     Received 6
     Foo 6
     Bar 5 Prime
     Foo 4
     Bar 3 Prime
     Foo 2 Prime
     Bar 1
     Foo 0
     ```

4. **Precise Timing:**
   - The program ensures that the output messages (`"Foo"` or `"Bar"`) are spaced exactly one second apart. The next message starts outputting one second after the previous message began.

5. **Dynamic Input Handling:**
   - The program can receive a new integer `N` at any time. When a new number is received:
     - If the input is `0`, the ESP32 restarts immediately.
     - The program acknowledges receipt of the new number.
     - The current sequence completes before starting the new sequence. In other words, it always finishes counting down to `"Foo 0"` before processing the next number.
     - The program processes sequences in the order they are received.
     - If more than 8 numbers are received while a sequence is being processed, the 9th and subsequent numbers are ignored, and the message `"Currently full"` is output.

6. **Assumptions:**
   - Valid input values for `N` are assumed, with `N < 2^31`.
   - The program is designed for a standard ESP32-S3 chip.

## Implementation Details

- **Multi-core Utilization:**
  - The program leverages the dual-core architecture of the ESP32. Core 0 handles even numbers and the prime check, while Core 1 handles odd numbers. This parallelism ensures efficient task handling and output timing.

- **Task Synchronization:**
  - FreeRTOS tasks are used to manage the different functionalities. A task for handling serial input runs concurrently with the tasks responsible for counting down and outputting messages. This ensures that the system remains responsive to new input while processing ongoing sequences.

- **Prime Number Check:**
  - A simple prime-checking algorithm is implemented to identify prime numbers during the countdown. This check runs only when necessary, optimizing the system's performance.

- **Robustness:**
  - The program includes error handling for invalid inputs and ensures that sequences are processed in order, even when multiple inputs are received rapidly.

## Usage

1. **Setup:**
   - Load the program onto an ESP32-S3 board using the Arduino IDE or PlatformIO.
   - Connect the ESP32 to a computer via USB for serial communication.

2. **Operation:**
   - Open a serial monitor at the correct baud rate (e.g., 115200).
   - Enter a positive integer to start the countdown sequence.
   - Enter additional numbers at any time to queue new sequences.
   - Enter `0` to restart the ESP32.

3. **Output:**
   - The serial monitor will display the countdown sequence as described, with `"Foo"`, `"Bar"`, and `"Prime"` messages appropriately spaced.

## Conclusion

This project effectively demonstrates the capabilities of the ESP32 and FreeRTOS for handling real-time tasks with precise timing requirements. By leveraging multi-core processing, the solution achieves efficient task management and meets the requirements of the "Ella Says 'Foo-Bar'" problem statement.

| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C6 | ESP32-H2 | ESP32-P4 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | -------- | -------- | -------- |

# _Sample project_

(See the README.md file in the upper level 'examples' directory for more information about examples.)

This is the simplest buildable example. The example is used by command `idf.py create-project`
that copies the project to user specified path and set it's name. For more information follow the [docs page](https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html#start-a-new-project)



## How to use example
We encourage the users to use the example as a template for the new projects.
A recommended way is to follow the instructions on a [docs page](https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html#start-a-new-project).

## Example folder contents

The project **sample_project** contains one source file in C language [main.c](main/main.c). The file is located in folder [main](main).

ESP-IDF projects are built using CMake. The project build configuration is contained in `CMakeLists.txt`
files that provide set of directives and instructions describing the project's source files and targets
(executable, library, or both). 

Below is short explanation of remaining files in the project folder.

```
├── CMakeLists.txt
├── main
│   ├── CMakeLists.txt
│   └── main.c
└── README.md                  This is the file you are currently reading
```
Additionally, the sample project contains Makefile and component.mk files, used for the legacy Make based build system. 
They are not used or needed when building with CMake and idf.py.
