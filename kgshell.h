#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>

#define TRUE 1
#define FALSE 0

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"
#define RESET "\033[0m"

typedef struct Args{
	int argc;
	char **argv;
} args_t;

typedef struct Command {
	int cmdc;
	args_t **args;
	char **seps;
} cmd_t;

void init();
char * getLine();
int checkBuiltIn(char arr[]);
cmd_t *makeCommand();
cmd_t **parseCommand(char *line);
void freeCmds(cmd_t **cmds);
void printCommand(cmd_t *cmd);
void printCommands(cmd_t **cmd);
args_t * getArgs(char *line);
void freeArgs(args_t *args);
void printArgs(args_t *args);
void handleCD(args_t *args);
void handleBuiltIn(args_t *args);
void executeCommand(cmd_t *cmd);
void _executeCommand(cmd_t *cmd);
void formatcwd();
void interruptHandler(int sig);
