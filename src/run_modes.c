#include "run_modes.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <setjmp.h>
#include <fcntl.h>
#include "command_execution.h"
#include "data_structures.h"
#include "parser_caller.h"
#include "utils.h"

/* Buffer for capturing a state of the program and then its restoration */
extern sigjmp_buf sigint_buf;

/* Try to run commands in -c mode. */
/* If executed, exit the program after execution */
/* Otherwise return 0 */
size_t
c_option_run(int argc, char *const *argv) {
	int c;
	while ((c = getopt(argc, argv, "c:")) != -1) {
		switch (c) {
		case 'c':
			if (!input_too_large(optarg)) {
				struct commands_handle *to_execute =
					parse(optarg, 1);
				// If error in parsing exit
				if (get_parser_exit_code() != 0)
					exit(get_parser_exit_code());
				execute_commands(to_execute);
				internal_exit();
			}
		}
	}
	return (0);
}

/* Read commands from file */
void
noninteractive_run(const char *filename) {
	int fd = open(filename, O_RDONLY);
	if (fd == -1) {
		fprintf(stderr,
			"An executable file: %s does not exist.\n",
			filename);
		exit(EXIT_FAILURE);
	}
	char *buffer;
	size_t nread;
	ssize_t line_size = 0;
	size_t line_num = 1;
	buffer = malloc(sizeof (char));
	check_allocation(buffer);
	while ((nread = read(fd, &buffer[line_size], 1)) > 0) {
		if (buffer[line_size] == '\n' || buffer[line_size] == 0x0) {
			buffer[line_size] = '\0';
			if (!input_too_large(buffer)) {
				struct commands_handle *to_execute =
					parse(buffer, line_num);
				// If error in parsing exit
				if (get_parser_exit_code() != 0)
					exit(get_parser_exit_code());
				execute_commands(to_execute);
			}
			// Reset everything and move on the next line
			free(buffer);
			line_num++;
			line_size = 0;
			buffer = malloc(sizeof (char));
			check_allocation(buffer);
		} else {
			line_size++;
			// Allocate space for a new char. New buffer size is
			// (line_size + 1) because line_size is indexed from 0
			char *tmp = realloc(buffer,
				(line_size + 1) * sizeof (char));
			check_allocation(tmp);
			buffer = tmp;
		}
	}
	free(buffer);
	close(fd);
	if (nread == -1) {
		fprintf(stderr,
			"A problem when reading file: %s has occured.\n",
			filename);
		exit(EXIT_FAILURE);
	}
	internal_exit();
}

void
interactive_mode_loop() {
	// Register sigint handler
	set_sigint_handler();
	// Configure readline to auto-complete paths when the tab key is hit.
	rl_bind_key('\t', rl_complete);
	char *input, *cwd, shell_prompt[100];
	while (1) { // While not exited by ^D
		// Create prompt string from user name
		// and current working directory.
		cwd = getcwd(NULL, 0);
		snprintf(shell_prompt, sizeof (shell_prompt), "mysh:%s$ ", cwd);
		// Set return point for case of ^C
		while (sigsetjmp(sigint_buf, 1) != 0) {
		}
		// Display prompt and read input
		input = readline(shell_prompt);
		// Check for EOF(^D)
		if (!input) // If EOF, exit with the last command's value
			internal_exit();
		if (!input_too_large(input)) {
			struct commands_handle *to_execute = parse(input, 1);
			// If error in parsing don't execute the line
			if (get_parser_exit_code() != 0)
				set_exit_code(get_parser_exit_code());
			else
				execute_commands(to_execute);
			// Add input to history.
			add_history(input);
		}
		free(cwd);
		free(input);
	}
}
