# Mini Shell

This project implements a Mini Shell, a simplified version of a Unix-like shell, built using C++, Lex, and Yacc. It supports command execution, input/output redirection, piping, background processes, signal handling, and wildcard expansion. The shell is developed incrementally through a structured approach, enhancing both parsing and process management functionalities.

## Features

- **Command Execution**: Execute simple and complex commands.
- **Input/Output Redirection**: Redirect input and output using `<`, `>`, `>>`, and `>>&`.
- **Piping**: Support for piping commands using `|`.
- **Background Processes**: Run commands in the background using `&`.
- **Signal Handling**: Handle signals like `Ctrl+C`.
- **Logging**: Log child process termination times.

## Files

- **command.cc**: Implementation of the `Command` and `SimpleCommand` classes.
- **command.h**: Header file for the `Command` and `SimpleCommand` classes.
- **shell.l**: Lex file for tokenizing the input.
- **shell.y**: Yacc file for parsing the input and building the command structure.
- **Makefile**: Build script for compiling the shell and example programs.
- **examples/**: Example programs demonstrating the use of the shell.

## Building the Project

To build the project, run the following command in the terminal:

```sh
make
```

# To run the shell, execute the following command:

```sh
./shell
```

# You can now enter commands to be executed by the shell.

Example Commands
Simple Command: ls
Input Redirection: cat < input.txt
Output Redirection: ls > output.txt
Append Output: ls >> output.txt
Piping: ls | grep shell
Background Process: sleep 10 &

# Signal Handling

The shell handles Ctrl+C to interrupt the current command and return to the prompt.

# Logging

The shell logs the termination time of child processes to log.txt.
