#include "commands.h"

int backgroundJobCount = 1; // global variable
Command lastBackgroundCommand;

void printCommand(Command command) {
    //printf("%s", command.name);
    for (int i = 0; command.argv[i] != NULL; i++) {
        printf(" %s", command.argv[i]);
    }
    printf("\n");
}

char* trimWhitespace(char* str) {
    int len = strlen(str);
    int actualStart = 0;
    int actualEnd = len - 1;

    //leading whitespaces
    while (actualStart < len && str[actualStart] == ' ') {
        actualStart++;
    }

    if (actualStart == len) {
        char* emptyString = malloc(1);
        emptyString[0] = '\0';
        return emptyString;
    }

    //trailing whitespaces
    while (actualEnd >= actualStart && (str[actualEnd] == ' ' || str[actualEnd] == '\n')) {
        actualEnd--;
    }

    int actualSize = actualEnd - actualStart + 1;
    char* trimmedWord = malloc((actualSize + 1) * sizeof(char));

    // create new string
    for (int i = 0; i < actualSize; i++) {
        trimmedWord[i] = str[actualStart + i];
    }
    trimmedWord[actualSize] = '\0';

    return trimmedWord;
}

Command parseCommand(char *input) {
    Command command;
    command.argv = malloc((MAX_ARGS + 1) * sizeof(char*)); 
    command.name = NULL;
    command.isBackground = false;
    command.isInputRedirect = false;
    command.isOutputRedirect = false;
    command.inputFile = NULL;
    command.outputFile = NULL;

    char *inputCopy = strdup(input);

    if (!command.argv || !inputCopy) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    char *outputRedirect = strchr(inputCopy, '>');
    char *inputRedirect = strchr(inputCopy, '<');

    if (outputRedirect) {
        *outputRedirect = '\0';
        command.isOutputRedirect = true;
        command.outputFile = trimWhitespace(outputRedirect + 1);
    }
    if (inputRedirect) {
        *inputRedirect = '\0';
        command.isInputRedirect = true;
        command.inputFile = trimWhitespace(inputRedirect + 1);
    }

    int currArgCount = 0;
    char* trimmedInput = trimWhitespace(inputCopy);
    char *token = strtok(trimmedInput, " \n");
    if (token != NULL) {
        command.name = strdup(trimWhitespace(token));
        command.argv[currArgCount++] = command.name;
    }

    while ((token = strtok(NULL, " \n")) != NULL && currArgCount < MAX_ARGS) {
        token = trimWhitespace(token);
        if (strcmp(token, "&") == 0) {
            command.isBackground = true;
            break;
        }
        command.argv[currArgCount++] = strdup(token);
    }

    command.argv[currArgCount] = NULL;

    //free input copy but not tokens
    free(inputCopy);
    return command;
}

int splitByPipe(char *input, char **commands) {
    int commandCount = 0;
    char *pipeToken = strtok(input, "|");

    while (pipeToken && commandCount < MAX_ARGS) {
        commands[commandCount] = pipeToken;
        // printf("%s\n", pipeToken);
        while (*pipeToken == ' ' || *pipeToken == '\t') {
            pipeToken++;
        }
        commands[commandCount] = trimWhitespace(pipeToken);
        // printf("%s\n", commands[commandCount]);
        commandCount++;
        pipeToken = strtok(NULL, "|");
    }
    commands[commandCount] = NULL;
    return commandCount;
}

void backgroundProcessHandler(int signum) {
    // printf("ENTERING WITH SIGNUM %d\n", signum);
    if (signum != SIGCHLD)
        return;
    int wstatus;
    pid_t pid;

    // reap all terminated child processes
    while ((pid = waitpid(-1, &wstatus, WNOHANG)) > 0) {
        // printf("RUNNING BACKGROUND PROCESS HANDLER\n");
        if (WIFEXITED(wstatus)) { // child process terminates properly?
            backgroundJobCount--;
            printf("\n[%d]  + Done       ", backgroundJobCount);
            for (int i = 0; lastBackgroundCommand.argv[i] != NULL; i++) {
                printf(" %s", lastBackgroundCommand.argv[i]);
            }
            printf("\nprompt_shell$ ");
            printf("\n");
            // printf("[Done] Process %d finished with status %d.\n", pid, WEXITSTATUS(wstatus));
        }
    }
}

void executeRedirectCommand(Command command, bool isExec) {
    // assume command has either > or <
    if (command.isOutputRedirect) { // redirect output to file
        char *outputFile = trimWhitespace(command.outputFile);
        //\n", outputFile);
        int fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            perror("ERROR: Unable to open or create file");
            exit(EXIT_FAILURE);
        }
        dup2(fd, 1);
        close(fd);
        if (isExec) {
            execvp(command.name, command.argv);
            perror("ERROR: execvp failed");
            exit(EXIT_FAILURE);
        }
    }
    else if (command.isInputRedirect) {
        char *inputFile = command.inputFile;
        //inputFile = "elements.txt";
        int fd = open(inputFile, O_RDONLY);
        if (fd == -1) {
            perror("ERROR: Unable to open or create file");
            exit(EXIT_FAILURE);
        }
        dup2(fd, 0);
        close(fd);
        if (isExec) {
            execvp(command.name, command.argv);
            perror("ERROR: execvp failed");
            exit(EXIT_FAILURE);
        }
    }
}

void executeSingleCommand(Command command) {
    pid_t pid;
    int wstatus;

    pid = fork();

    if (pid == 0) { //child process
        if (command.isInputRedirect || command.isOutputRedirect) {
            executeRedirectCommand(command, true);
        } else {
            execvp(command.name, command.argv); 
            perror("ERROR: execvp failed");
            exit(EXIT_FAILURE);
        }
    } else if (pid > 0) { //parent
        if (command.isBackground) {
            command.backgroundJobIndex = backgroundJobCount++;
            lastBackgroundCommand = command;
            printf("[%d] %d\n", command.backgroundJobIndex, pid);
            signal(SIGCHLD, backgroundProcessHandler);
        } else {
            signal(SIGCHLD, SIG_DFL);
            waitpid(pid, &wstatus, 0);
            signal(SIGCHLD, backgroundProcessHandler);
        }
    } else {
        perror("ERROR: fork failed");
        exit(EXIT_FAILURE);
    }
}

void executePipedCommands(char **commands, int commandCount) {
    int prev_fd = -1;  // Used for reading from previous pipe
    pid_t pid;
    Command currCommand;

    for (int i = 0; i < commandCount; i++) {
        int pipefd[2];
        currCommand = parseCommand(commands[i]);

        if (i < commandCount - 1) {
            if (pipe(pipefd) == -1) {
                perror("ERROR: pipe failed");
                exit(1);
            }
        }

        pid = fork();
        if (pid == 0) {  // child process
            // input redirection of first command || output redirection of last command
            if ((i == 0 && currCommand.isInputRedirect) || (i == commandCount - 1 && currCommand.isOutputRedirect)) {
                executeRedirectCommand(currCommand, false);
            }

            if (prev_fd != -1) {
                dup2(prev_fd, STDIN_FILENO);
                close(prev_fd);
            }

            if (i < commandCount - 1) {
                close(pipefd[0]);
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[1]);
            }

            execvp(currCommand.name, currCommand.argv);
            perror("ERROR: execvp failed.");
            exit(EXIT_FAILURE);
        } else if (pid > 0) {  // Parent Process
            int wstatus;
            signal(SIGCHLD, SIG_DFL);
            waitpid(pid, &wstatus, 0);

            if (prev_fd != -1) {
                close(prev_fd);
            }
            if (i < commandCount - 1) {
                close(pipefd[1]);
                prev_fd = pipefd[0];
            }
        } else {
            perror("ERROR: fork failed");
            exit(EXIT_FAILURE);
        }
    }
    // test if piping works
    /*for (int i = 0; i < commandCount; i++) {
        printf("Piped command: %s\n", commands[i]);
    }*/
}
