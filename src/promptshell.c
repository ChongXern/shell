#include "promptshell.h"
#include "threads.h"
#include "commands.h"

void runShell(bool hasPrompt) {
    char inputBuffer[MAX_INPUT_LEN];
    char **commands = malloc(MAX_ARGS * sizeof(char *));

    while (true) {
        if (hasPrompt) {
            printf("prompt_shell$ ");
        }

        if (fgets(inputBuffer, sizeof(inputBuffer), stdin) != NULL) { // successfully read?
            if (strlen(inputBuffer) == 0) { // pressed enter
                continue;
            }

            int commandCount = splitByPipe(inputBuffer, commands);

            if (commandCount > 1) { // piped commands
                for (int i = 0; i < commandCount; i++) {
                    commands[i] = trimWhitespace(commands[i]);
                }
                executePipedCommands(commands, commandCount);
            } else {
                // printf("SINGLE COMMAND EXECUTING\n");
                Command firstCommand = parseCommand(commands[0]);
                executeSingleCommand(firstCommand);
            }
        } else {
            printf("\n");
            break;
        }
    }
}

int main(int argc, char* argv[]) {
    bool hasPrompt = true;
    if (argc > 1 && strcmp(argv[1], "-n") == 0) {
        hasPrompt = false;
    }
    runShell(hasPrompt);
    return 0;
}
