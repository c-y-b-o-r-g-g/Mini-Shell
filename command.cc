#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>
#include <ctime>
#include <fcntl.h> // Include for open and O_* constants

#include "command.h"

// SimpleCommand Constructor
int yyparse(void);
SimpleCommand::SimpleCommand()
{
	// Create available space for 5 arguments
	_numberOfAvailableArguments = 5;
	_numberOfArguments = 0;
	_arguments = (char **)malloc(_numberOfAvailableArguments * sizeof(char *));
	if (!_arguments)
	{
		perror("Failed to allocate memory for arguments");
		exit(EXIT_FAILURE);
	}
}

void childIsTerminated()
{
	FILE *fp = fopen("log.txt", "a");
	if (fp == NULL)
	{
		fp = fopen("log.txt", "w");
		if (fp == NULL)
		{
			perror("Failed to create log file");
			exit(EXIT_FAILURE);
		}
	}
	time_t t;
	t = time(NULL);
	tm *time = gmtime(&t);
	fprintf(fp, "Child process was terminated at %d:%d:%d\n", time->tm_hour, time->tm_min, time->tm_sec);
	fclose(fp);
}

void ctrlcHandler(int sig)
{
	printf(" Ctrl + C Handled\n");
	Command::_currentCommand.prompt();
}

// Insert an argument into SimpleCommand
void SimpleCommand::insertArgument(char *argument)
{
	if (_numberOfAvailableArguments == _numberOfArguments + 1)
	{
		// Double the available space
		_numberOfAvailableArguments *= 2;
		_arguments = (char **)realloc(_arguments, _numberOfAvailableArguments * sizeof(char *));
	}

	// Store the argument
	_arguments[_numberOfArguments] = strdup(argument); // Duplicate the argument string
	_arguments[_numberOfArguments + 1] = NULL;		   // Add NULL argument at the end
	_numberOfArguments++;
}

// Command Constructor
Command::Command()
{
	// Create available space for one simple command
	_numberOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)malloc(_numberOfAvailableSimpleCommands * sizeof(SimpleCommand *));
	if (!_simpleCommands)
	{
		perror("Failed to allocate memory for simple commands");
		exit(EXIT_FAILURE);
	}
	_numberOfSimpleCommands = 0;
	_outFile = nullptr;
	_inputFile = nullptr;
	_errFile = nullptr;
	_background = 0;
}

void Command::insertSimpleCommand(SimpleCommand *simpleCommand)
{

	// Check if simpleCommand is null
	if (!simpleCommand)
	{
		fprintf(stderr, "Error: simpleCommand is null.\n");
		return;
	}

	// Check if _simpleCommands is null
	if (!_simpleCommands)
	{
		fprintf(stderr, "Error: _simpleCommands array is null.\n");
		return;
	}

	if (_numberOfAvailableSimpleCommands == _numberOfSimpleCommands)
	{
		_numberOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **)realloc(_simpleCommands,
													_numberOfAvailableSimpleCommands * sizeof(SimpleCommand *));
		if (!_simpleCommands)
		{
			perror("Failed to reallocate memory for simple commands");
			exit(EXIT_FAILURE);
		}
	}

	// Insert the simple command
	_simpleCommands[_numberOfSimpleCommands] = simpleCommand;
	_numberOfSimpleCommands++;

	// Debug print to confirm insertion
	// print();
	// return;
	// execute();
	// clear();
	// prompt();
	// Remove clear() and prompt() here
	// print(); // Optionally keep this to see commands
}

// Clear the command state
void Command::clear()
{
	for (int i = 0; i < _numberOfSimpleCommands; i++)
	{
		for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
		{
			free(_simpleCommands[i]->_arguments[j]);
		}

		free(_simpleCommands[i]->_arguments);
		free(_simpleCommands[i]);
	}

	if (_outFile)
	{
		free(_outFile);
	}

	if (_inputFile)
	{
		free(_inputFile);
	}

	if (_errFile)
	{
		free(_errFile);
	}

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
}

// Print command details
void Command::print()
{
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------\n");

	for (int i = 0; i < _numberOfSimpleCommands; i++)
	{
		printf("  %-3d ", i);
		for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
		{
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[j]);
		}
		printf("\n"); // Ensure each command is printed on a new line
	}

	printf("\n\n");
	printf("  Output       Input        Error        Background\n");
	printf("  ------------ ------------ ------------ ------------\n");
	printf("  %-12s %-12s %-12s %-12s\n",
		   _outFile ? _outFile : "default",
		   _inputFile ? _inputFile : "default",
		   _errFile ? _errFile : "default",
		   _background ? "YES" : "NO");
	printf("\n\n");
}

// Execute the command
void Command::execute()
{
	// Don't do anything if there are no simple commands
	print();
	if (_numberOfSimpleCommands == 0)
	{
		prompt();
		return;
	}

	int tmpin = dup(0);
	int tmpout = dup(1);
	int tmperr = dup(2);
	int fdin, fdout, fderr;
	int fdapp;
	int pid;
	int fdpipe[2];

	if (pipe(fdpipe) == -1)
	{
		perror("Piping Error");
		exit(2);
	}

	if (_inputFile)
	{
		fdin = open(_inputFile, O_RDONLY);
		if (fdin < 0)
		{
			perror("open input file");
			exit(1);
		}
	}
	else
	{
		fdin = dup(tmpin);
	}

	// Handle error redirection
	if (_errFile)
	{
		fderr = open(_errFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
		if (fderr < 0)
		{
			perror("open error file");
			exit(1);
		}
		dup2(fderr, 2);
	}
	else
	{
		fderr = dup(tmperr);
	}

	// For every simple command, fork a new process
	for (int i = 0; i < _numberOfSimpleCommands; i++)
	{
		// Redirect input
		dup2(fdin, 0);
		close(fdin);

		int n;
		char *command = _simpleCommands[i]->_arguments[0];

		if (!strcasecmp(command, "exit"))
		{
			printf("Exiting..\nGoodbye!\n");
			exit(0);
		}

		if (!strcasecmp(command, "cd"))
		{
			if (_simpleCommands[i]->_numberOfArguments > 1)
			{
				char *path = _simpleCommands[i]->_arguments[1];
				n = strlen(path);
				// char path[n + 1];
				// strcpy(path, path.c_str());
				strcpy(path, path);
				if (chdir(path))
				{
					printf("Directory not found\n");
				}
				break;
			}
			else
			{
				chdir(getenv("HOME"));
				break;
			}
		}

		if (i == 0)
		{
			if (_inputFile)
			{
				fdin = open(_inputFile, O_RDONLY);
				if (fdin < 0)
				{
					perror("error in input file");
					exit(1);
				}
				dup2(fdin, 0);
				close(fdin);
			}
			else
			{
				fdin = dup(tmpin);
				// dup2(fdin, 0);
			}

			// Handle error redirection
			// if (_errFile)
			// {
			// 	fderr = open(_errFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
			// 	if (fderr < 0)
			// 	{
			// 		perror("open error file");
			// 		exit(1);
			// 	}
			// }
			// else
			// {
			// 	fderr = dup(tmperr);
			// }
		}
		if (i != 0)
		{
			dup2(fdpipe[0], 0);
			close(fdpipe[0]);
			close(fdpipe[1]);
			if (pipe(fdpipe) == -1)
			{
				perror("Piping Error");
				exit(2);
			}
		}

		// Setup output redirection or pipe
		if (i == _numberOfSimpleCommands - 1)
		{

			if (_appendFile)
			{
				fdapp = open(_appendFile, O_WRONLY | O_CREAT | O_APPEND, 0666);
				if (fdout < 0)
				{
					perror("error in output file");
					exit(1);
				}
				dup2(fdout, 1);
				close(fdout);
			}
			// Last command
			if (_outFile)
			{
				fdout = open(_outFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
				if (fdout < 0)
				{
					perror("open output file");
					exit(1);
				}
				dup2(fdout, 1);
				close(fdout);
			}

			if (_errFile)
			{
				fderr = open(_errFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
				if (fderr < 0)
				{
					perror("error redirecting output");
					exit(1);
				}
				dup2(fderr, 2);
				close(fderr);
			}
			if (_appendFile != 0)
			{
				dup2(fdapp, 1);
			}
			else if (_outFile != 0)
			{
				dup2(fdout, 1);
			}
			else
			{
				dup2(tmpout, 1);
			}
		}
		else
		{
			// Not the last command
			// pipe(fdpipe);
			// fdout = fdpipe[1];
			// fdin = fdpipe[0];
			dup2(fdpipe[1], 1);
		}

		// Redirect output
		// dup2(fdout, 1);
		// close(fdout);

		// Redirect error
		// dup2(fderr, 2);

		// Fork the process to execute the command
		pid = fork();
		if (pid == 0)
		{
			// Child process
			if (_outFile)
			{
				close(fdout);
			}
			if (_inputFile)
			{
				close(fdin);
			}
			if (_appendFile)
			{
				close(fdapp);
			}
			if (_errFile)
			{
				close(fderr);
			}
			childIsTerminated();
			close(fdin);
			close(fdout);
			close(fderr);

			execvp(_simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments);
			perror("Execution Error");
			printf("Child Process\n");
			_exit(1); // Exit the child process
		}
		else if (pid < 0)
		{
			perror("fork");
			exit(1);
		}

		// Parent process
		if (_background == 0)
		{
			waitpid(pid, nullptr, 0); // Wait for child process if not background
		}
	}

	// Restore original stdin, stdout, and stderr
	dup2(tmpin, 0);
	dup2(tmpout, 1);
	dup2(tmperr, 2);

	if (_inputFile != 0)
	{
		close(fdin);
	}
	if (_appendFile != 0)
	{
		close(fdapp);
	}
	if (_outFile != 0)
	{
		close(fdout);
	}
	if (_errFile != 0)
	{
		close(fderr);
	}

	close(tmpin);
	close(tmpout);
	close(tmperr);

	// waitpid(ret, 0, 0);
	// exit(2);

	// Clear to prepare for the next command
	clear();
	prompt();
	// return;
	// main();
}

// Prompt for the shell
void Command::prompt()
{
	printf("myshell>");
	fflush(stdout);
}

// Static members initialization
Command Command::_currentCommand;
SimpleCommand *Command::_currentSimpleCommand;

// Function prototype for parsing commands

// Main function
int main()
{
	signal(SIGINT, ctrlcHandler);
	Command::_currentCommand.prompt();	// Display the prompt
	yyparse();							// Parse the command input
	Command::_currentCommand.execute(); // Execute the command
	return 0;
}
