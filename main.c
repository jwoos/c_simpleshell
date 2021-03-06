#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "shell.h"


volatile int PID;


void sigactionHandler(int sig) {
	switch (sig) {
		case SIGINT: {
			if (PID > 0) {
				kill(PID, SIGINT);
				/*wait(NULL);*/
				/*PID = 0;*/
				flush();
			} else {
				flush();
				prompt();
			}
			break;
		}

		default: {
			writeStdout("Nothing matched\n", 15);
			break;
		}
	}
}

void handleSignals(void) {
	struct sigaction act;
	act.sa_handler = &sigactionHandler;
	// don't reset the handler
	act.sa_flags = SA_RESTART;

	if (sigaction(SIGINT, &act, NULL) < 0) {
		printError("signal handler not registered properly", 1);
	}
}

int main(int argc, char* argv[]) {
	handleSignals();

	CommandLine* cl;
	char* input;

	while (true) {
		prompt();

		input = readStdin();
		cl = parseCommand(input);
		Vector* commands = cl -> commands;

		for (uint32_t i = 0; i < commands -> size; i++) {
			Command* cmd = vectorGet(commands, i);
			char* base = cmd -> cmd;
			Vector* args = cmd -> args;

			bool cont = builtIns(base, vectorGet(args, 1));

			if (!cont && base[0] != '\0') {
				int status;

				PID = fork();

				if (PID < 0) {
					printError("Error forking process", 0);
				}

				if (PID == 0) {
					// child
					if (execvp(base, (char**)args -> array) == -1) {
						printError("Command failure", 1);
					}
				} else {
					// parent
					wait(&status);
				}
			}
		}

		cleanUpCommand(cl);
	}

	return EXIT_SUCCESS;
}
