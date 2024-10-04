#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#define MAX_ARG_LEN 32
#define MAX_INPUT_LEN 512
#define MAX_ARGS 10

typedef struct {
    char* name;
    char** argv;
    bool isBackground;
    int backgroundJobIndex;
    bool isInputRedirect; // <
    bool isOutputRedirect; // >
    char* inputFile;
    char* outputFile;
} Command;

void printCommand(Command command);
char* trimWhitespace(char* str);
Command parseCommand(char *input);

int splitByPipe(char *input, char **commands);
void backgroundProcessHandler(int signum);
void executeRedirectCommand(Command command, bool isExec);
void executeSingleCommand(Command command);
void executePipedCommands(char **commands, int commandCount);

#endif