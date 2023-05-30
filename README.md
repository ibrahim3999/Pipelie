# Single Threaded Pipeline

This program demonstrates a single-threaded pipeline implementation using active objects. It processes a series of numbers through multiple stages, where each stage performs a specific task on the numbers.

## Description

The program utilizes active objects to implement a single-threaded pipeline architecture. The pipeline consists of four stages:

1. **First Stage**: Checks if a number is prime or not.
2. **Second Stage**: Adds 11 to the number.
3. **Third Stage**: Subtracts 13 from the number.
4. **Fourth Stage**: Adds 2 to the number.

Each stage is represented by an active object and enqueues the numbers into the next stage's queue after performing its specific task.

The program accepts a positive number of iterations as a command-line argument. It generates random numbers and enqueues them into the first stage's queue for processing.

## Dependencies

The program requires the following dependencies:

- `gcc` (GNU Compiler Collection)
- `pthread` (POSIX Threads)

Ensure that these dependencies are installed on your system before compiling and running the program.

## Usage

To compile the program, use the provided Makefile. Open a terminal, navigate to the directory containing the source code (`st_pipeline.c`), and run the following command:

```shell
make
