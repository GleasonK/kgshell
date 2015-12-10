/* File: kgshell.c
 * Date: 12/07/2015
 * Author: Kevin Gleason
 */

#include "kgshell.h"

const char * const builtin[] = { "cd", "fg", "exit"};
char cwd[MAXPATHLEN];
char *HOME, *USER;

int waiting;
int fd_in;

int main(){
	pid_t proc[2];
	cmd_t *cmd, **cmds;
	args_t *args;
	char *line;
	int i;

	init();

	while(1){
		getcwd(cwd,sizeof(cwd));
		formatcwd();
		printf(KCYN "%s"  RESET ":" KMAG "%s> " RESET, USER, cwd); // The prompt
		line = getLine();
		cmds=parseCommand(line);
		//printCommands(cmds);
		i=0;

		while ((cmd=*(cmds+i++)) != NULL){
			args = *(cmd->args);
			if (args->argc==0) continue;
			if (strcmp(*(args->argv),"exit") == 0){
				puts("Exiting kgsh...");
				exit(1);
			}

			if (checkBuiltIn(*(args->argv))) { //TODO: Split string and send it in
				handleBuiltIn(args);
			} else { // Normal command
				executeCommand(cmd);
			}
		}
		free(line);
		freeCmds(cmds);
	}
}

// Setup the shell
void init(){
	HOME = getenv("HOME");
	USER = getenv("USER");
	fd_in=0;
	signal(SIGINT, interruptHandler); //TODO UNCOMMENT
}

char * getLine(){
	char c,*line = (char *)malloc(sizeof(char));
	int i=0;
	c = getchar();
	while (c != '\n'){
		*(line+i++)=c;
		c=getchar();
		line=(char *)realloc(line, (i+1) * sizeof(char));
	}
	*(line+i)='\0';
	return line;
}

cmd_t * makeCommand(){
	cmd_t *cmd = (cmd_t *)malloc(sizeof(cmd_t));
	cmd->cmdc=0;
	cmd->seps = (char **)malloc(sizeof(char *) * 10); // TODO: Currently max 10
	cmd->args = (args_t **)malloc(sizeof(args_t *) * 10);
	return cmd;
}

cmd_t ** parseCommand(char *line){
	char tmp[3];
	char *c = line;
	cmd_t **cmds = (cmd_t **)malloc(sizeof(cmd_t *) * 10);
	cmd_t *cmd = makeCommand();
	int cmdCt = 0;
	int sepCt = 0;
	int argCt = 0;
	tmp[2]='\0';
	while (*c != '\0'){
		if (*c=='|' || *c=='<' || *c=='>'){
			tmp[0]=*c;
			tmp[1]='\0'; // In case only 1 char sep
			*(cmd->seps+sepCt)=(char *) malloc(sizeof(char) * 3);
			if (*c!='|' && *c==*(c+1)){ // for << and >>
				tmp[1]=*c;
				*c='\0';
				c++;
			}
			strcpy(*(cmd->seps+sepCt),tmp);
			sepCt++;
			*c='\0';
			*(cmd->args+argCt)=(args_t *)malloc(sizeof(args_t));
			*(cmd->args+argCt)=getArgs(line);
			argCt++;
			line=c+1;
		} else if  (*c=='&' && *(c+1)=='&') {
			*(c++)='\0'; *c='\0';
			*(cmd->args+argCt)=(args_t *)malloc(sizeof(args_t));
			*(cmd->args+argCt)=getArgs(line);
			argCt++;
			line=c+1;

			cmd->cmdc=argCt;
			*(cmds+cmdCt++)=cmd;
			argCt=0;sepCt=0;
			cmd=makeCommand();
		}
		c++;
	}
	*(cmd->args+argCt)=(args_t *)malloc(sizeof(args_t));
	*(cmd->args+argCt)=getArgs(line);
	argCt++;
	cmd->cmdc=argCt;
	*(cmds+cmdCt)=cmd;
	*(cmds+cmdCt+1)=NULL;
	return cmds;
}

void freeCmds(cmd_t **cmds){
	int i,j=0;
	cmd_t *cmd;

	while ((cmd=*(cmds+j++)) != NULL){
		for(i=0; i<cmd->cmdc;i++){
			freeArgs(*(cmd->args+i));
			if (i+1 < cmd->cmdc) // one less sep than arg
				free(*(cmd->seps+i));
		}	
		free(cmd->args);
		free(cmd->seps);
		free(cmd);
	}
	free(cmds);
}

void printCommand(cmd_t *cmd){
	int i;
	args_t *arg;
	printf("cmdc: %d\n",cmd->cmdc);
	for(i=0; i<cmd->cmdc;i++){
		arg=*(cmd->args+i);
		printArgs(arg);
		if (i+1 < cmd->cmdc){
			printf("Sep: %s\n", *(cmd->seps+i));
		}
	}
}

void printCommands(cmd_t **cmds){
	int i=0;
	cmd_t *cmd;

	while ((cmd=*(cmds+i++)) != NULL){
		printCommand(cmd);
	}
}

args_t * getArgs(char *line){
	args_t *args = (args_t *)malloc(sizeof(args_t));
	args->argc = 0;
	args->argv = NULL;
	char *token, **argv=NULL;
	const char delim[2] = " ";
	int i=0, tokLen;

	token=strtok(line,delim);
	argv = (char **)malloc(sizeof(char *));
	while(token != NULL){
		argv=(char **)realloc(argv, i+2 * sizeof(char *)); //Add spaces
		tokLen = strlen(token) + 1;
		*(argv+i)= (char *)malloc(sizeof(tokLen));
		strcpy(*(argv+i),token);
		i++;
		token = strtok(NULL,delim);
	}
	*(argv+i)  = NULL;
	args->argv = argv;
	args->argc = i;
	return args;
}

void printArgs(args_t *args){
	int i;
	printf("argc: %d\n",args->argc);
	printf("argv: [");
	for (i=0; i<args->argc; i++){
		printf("%s", *(args->argv+i));
		if (i+1 < args->argc) printf(", ");
	}
	printf("]\n");
}

void freeArgs(args_t *args){
	int i;
	// Free individual strings first
	for (i=0; i<args->argc; i++){
		free(*(args->argv+i));
	}
	free(args); // Free args struct
}

int checkBuiltIn(char *cmd){
	int i, items;
	items = sizeof(builtin)/sizeof(builtin[0]);
	for (i=0; i<items; i++){
		if (strcmp(builtin[i],cmd)==0){
			return 1;
		}	
	}
	return 0;
}

void handleBuiltIn(args_t *args){
	char *cmd;
	cmd = *(args->argv);
	if (strcmp(cmd,"cd")==0){
		handleCD(args);
	}
}

void handleCD(args_t *args){
	char *path, *tmp=NULL;
	int l1, l2;
	if (args->argc < 2){
		fprintf(stderr,"cd [path]\n");
		return;
	}
	path = *(args->argv+1);
	if (*path == '~'){ // Starts with ~
		l1   = strlen(HOME);
		tmp  = (char *)malloc(strlen(HOME)+strlen(path));
		strcpy(tmp, HOME);
		strcat(tmp,path+1);
		path = tmp;
	}

	if (chdir(path) != 0) {
		fprintf(stderr, "Directory: %s%s could not be found\n", cwd, path);
	}

	if (tmp != NULL) // Cleanup if tmp was needed
		free(tmp);
}

void executeCommand(cmd_t *cmd){
	args_t *args;
	pid_t pid;
	int status, err, background=FALSE;

	args=*(cmd->args+cmd->cmdc-1); // the last exec, check for bg

	if (**(args->argv+args->argc-1)=='&'){ // If last arg is &
		background = TRUE;
		*(args->argv+args->argc-1) = NULL;
	}
	if((pid = fork())==0){ // In Child 1
		// In child, process command
		_executeCommand(cmd);
	}
	if (background)
		printf("[pid] %d\n", pid);
	else {
		waiting=TRUE;
		waitpid(pid,&status,0);
		waiting=FALSE;
	}
}

void _executeCommand(cmd_t *cmd){
	args_t *args, *tmpArg;
	int file;
	char *sep;
	int err, status, access, permission;
	int p[2];
	pid_t pid[2];

	// printCommand(cmd);

	args = *(cmd->args++); // Get current arg and shift
	cmd->cmdc--;

	while(cmd->cmdc > 0){
		sep = *(cmd->seps++); // get current and increment.
		if (*sep=='<' || *sep=='>'){
			tmpArg=*(cmd->args++); cmd->cmdc--;
			permission=S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IRUSR;
			access=*sep=='<' ? O_RDONLY // If stdin, rdonly
				: O_CREAT | (*(sep+1)=='>' ? O_APPEND | O_WRONLY : O_WRONLY | O_TRUNC);
			if((file=open(*(tmpArg->argv), access, permission))==-1){
				perror("> File creation error");
				exit(0);
			}
			if (*sep=='<'){
				fd_in = file;
				fclose(stdin);
			}
			else 
				fclose(stdout);
			dup(file);
		} else if (*sep=='|'){
			fd_in = p[0];
			pipe(p);
			if (fork()==0){ // Outputs to pipe
				fclose(stdout); close(p[0]);
				dup(p[1]);
				if (execvp(*(args->argv),args->argv)==-1){
					err = errno;
					fprintf(stderr, "Error %d - Command %s not found.\n", err, *(args->argv));
				}
				close(p[1]);
				exit(0);
			}
			if (fork()==0){ // Receives from pipe
				if (fd_in!=0) close(fd_in); 
				else fclose(stdin); // For multiple pipes
				close(p[1]);
				dup(p[0]);
				fd_in=p[0];
				_executeCommand(cmd);
				close(p[0]);
				exit(0);
			}
			close(p[0]); close(p[1]); // Parent closes both ends
			wait(&status); wait(&status); // Wait for both children
			exit(0);
		} 	
	}
	if (execvp(*(args->argv),args->argv)==-1){
		err = errno;
		fprintf(stderr, "Error %d - Command %s not found.\n", err, *(args->argv));
	}
	exit(0);
}

// Format the working directory of the shell
void formatcwd(){
	char *p;
	int len, i;

	len = strlen(HOME);
	if(strncmp(cwd, HOME, len) == 0) {
		p = cwd + len;
		i=0;
		cwd[i++]='~';
		while(*p != '\0'){
			cwd[i++]=*p++;
		}
		cwd[i]='\0';
	}
}

void interruptHandler(int sig){
	printf("\n");
	if (!waiting)
		printf(KCYN "%s"  RESET ":" KMAG "%s> " RESET, USER, cwd); // The prompt
	fflush(stdout);
}

