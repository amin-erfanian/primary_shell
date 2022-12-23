#include <stdio.h> 
#include <string.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/types.h> 
#include <sys/wait.h> 
#include <readline/readline.h> 
#include <readline/history.h> 
#include <signal.h>
#include <setjmp.h>  
#include <fcntl.h> 
#include <sys/stat.h> 

#define MAXCOM 1000 // max number of letters to be supported 
#define MAXLIST 100 // max number of commands to be supported 
  
// Clearing the shell using escape sequences 
#define clear() printf("\033[H\033[J") 
  
static sigjmp_buf env;
void sig_handler(int signo) {
    siglongjmp(env, 42);
}

// Greeting shell during startup 
  
// Function to take input 
int takeInput(char* str) 
{ 
    char* buf; 
    char cwd[1024]; 
    getcwd(cwd, sizeof(cwd)); 
    printf("prompt:%s> ", cwd); 
    buf = readline(""); 
    if (buf == NULL) {
        printf("\n");
        exit(1);
    }
    if (strlen(buf) != 0) { 
        add_history(buf); 
        strcpy(str, buf); 
        return 0; 
    } else { 
        printf("\n");
        return 1; 
    }
} 
  
// Function where the system command is executed 
void execArgs(char** parsed) 
{ 
    // Forking a child 
    pid_t pid = fork();  
  
    if (pid == -1) { 
        printf("Failed forking child..\n"); 
        return; 
    } else if (pid == 0) { 
        if (execvp(parsed[0], parsed) < 0) { 
            printf("Could not execute command..\n"); 
        } 
        exit(0); 
    } else { 
        // waiting for child to terminate 
        wait(NULL);  
        return; 
    } 
} 
  
// Function where the piped system commands is executed 
void execArgsPiped(char** parsed, char** parsedpipe) 
{ 
    // 0 is read end, 1 is write end 
    int pipefd[2];  
    pid_t p1, p2; 
  
    if (pipe(pipefd) < 0) { 
        printf("\nPipe could not be initialized"); 
        return; 
    } 
    p1 = fork(); 
    if (p1 < 0) { 
        printf("\nCould not fork"); 
        return; 
    } 
  
    if (p1 == 0) { 
        // Child 1 executing.. 
        // It only needs to write at the write end 
      	signal(SIGINT, SIG_DFL);
        close(pipefd[0]); 
        dup2(pipefd[1], STDOUT_FILENO); 
        close(pipefd[1]); 
        if (execvp(parsed[0], parsed) < 0) { 
            printf("Could not execute command 1..\n"); 
            exit(0); 
        } 
    } else { 
        // Parent executing 
        p2 = fork(); 
  
        if (p2 < 0) { 
            printf("\nCould not fork"); 
            return; 
        } 
  
        // Child 2 executing.. 
        // It only needs to read at the read end 
        if (p2 == 0) { 
            signal(SIGINT, SIG_DFL);
            close(pipefd[1]); 
            dup2(pipefd[0], STDIN_FILENO); 
            close(pipefd[0]); 
            if (execvp(parsedpipe[0], parsedpipe) < 0) { 
                printf("Could not execute command 2..\n"); 
                exit(0); 
            } 
        } else { 
            // parent executing, waiting for two children 
            // wait(NULL); 
            wait(NULL); 
        } 
    } 
} 
  
// Help command builtin 
void openHelp() 
{ 
    puts(
        "\nList of Commands supported:"
        "\n>cd"
        "\n>ls"
        "\n>exit"
        "\n>all other general commands available in UNIX shell"
        "\n>pipe handling"
        "\n>improper space handling"); 
  
    return; 
} 
  
// Function to execute builtin commands 
int ownCmdHandler(char** parsed) 
{ 

    int fd; 
    char * myfifo = "/tmp/myfifo"; 
    char arr1[80], arr2[80]; 
    char str1[80], str2[80]; 
    int NoOfOwnCmds = 6, i, switchOwnArg = 0; 
    char* ListOfOwnCmds[NoOfOwnCmds]; 
    char* username; 

    ListOfOwnCmds[0] = "quit"; 
    ListOfOwnCmds[1] = "cd"; 
    ListOfOwnCmds[2] = "help"; 
    ListOfOwnCmds[3] = "hello"; 
    ListOfOwnCmds[4] = "sendmsg"; 
    ListOfOwnCmds[5] = "getmsg"; 
  
    for (i = 0; i < NoOfOwnCmds; i++) { 
        if (strcmp(parsed[0], ListOfOwnCmds[i]) == 0) { 
            switchOwnArg = i + 1; 
            break; 
        } 
    }
    
    switch (switchOwnArg) { 
    case 1: 
        printf("\nGoodbye\n"); 
        exit(0); 
    case 2: 
        chdir(parsed[1]); 
        return 1; 
    case 3: 
        openHelp(); 
        return 1; 
    case 4: 
        username = getenv("USER"); 
        printf("\nHello %s.\nMind that this is "
            "not a place to play around."
            "\nUse help to know more..\n", 
            username); 
        return 1; 
    case 5:
    
        mkfifo(myfifo, 0666); 
    
        while (1) 
        { 
            fd = open(myfifo, O_WRONLY); 
    
            fgets(arr2, 80, stdin); 
    
            write(fd, arr2, strlen(arr2)+1); 
            close(fd); 
    
            fd = open(myfifo, O_RDONLY); 
    
            read(fd, arr1, sizeof(arr1)); 
    
            printf("User2: %s\n", arr1); 
            close(fd); 
        }
    case 6:
        mkfifo(myfifo, 0666); 
    
        while (1) 
        { 
            fd = open(myfifo,O_RDONLY); 
            read(fd, str1, 80); 
    
            printf("User1: %s\n", str1); 
            close(fd); 
    
            fd = open(myfifo,O_WRONLY); 
            fgets(str2, 80, stdin); 
            write(fd, str2, strlen(str2)+1); 
            close(fd); 
        }
    default: 
        break; 
    } 
  
    return 0; 
} 
  
// function for finding pipe 
int parsePipe(char* str, char** strpiped) 
{ 
    int i; 
    for (i = 0; i < 2; i++) { 
        strpiped[i] = strsep(&str, "|"); 
        if (strpiped[i] == NULL) 
            break; 
    } 
  
    if (strpiped[1] == NULL) 
        return 0; // returns zero if no pipe is found. 
    else { 
        return 1; 
    } 
} 
  
// function for parsing command words 
void parseSpace(char* str, char** parsed) 
{ 
    int i; 
  
    for (i = 0; i < MAXLIST; i++) { 
        parsed[i] = strsep(&str, " "); 
  
        if (parsed[i] == NULL) 
            break; 
        if (strlen(parsed[i]) == 0) 
            i--; 
    } 
} 
  
int processString(char* str, char** parsed, char** parsedpipe) 
{ 
  
    char* strpiped[2]; 
    int piped = 0; 
  
    piped = parsePipe(str, strpiped); 
  
    if (piped) { 
        parseSpace(strpiped[0], parsed); 
        parseSpace(strpiped[1], parsedpipe); 
  
    } else { 
  
        parseSpace(str, parsed); 
    } 
  
    if (ownCmdHandler(parsed)) 
        return 0; 
    else
        return 1 + piped; 
} 
  
int main(int argc, char* argv[]) 
{ 
    char inputString[MAXCOM], *parsedArgs[MAXLIST]; 
    char* parsedArgsPiped[MAXLIST]; 
    int execFlag = 0; 
    if (argc == 1) {

        signal(SIGINT, sig_handler);
        while (1) { 
            if (sigsetjmp(env,1) == 42) {
                printf("\n");
            }
            // take input 
            if (takeInput(inputString)) 
                continue; 
            // process 
            execFlag = processString(inputString, 
            parsedArgs, parsedArgsPiped); 
            // execflag returns zero if there is no command 
            // or it is a builtin command, 
            // 1 if it is a simple command 
            // 2 if it is including a pipe. 
    
            // execute 
            if (execFlag == 1) 
                execArgs(parsedArgs); 
    
            if (execFlag == 2) 
                execArgsPiped(parsedArgs, parsedArgsPiped); 
        } 
    } else if (argc == 2) {
        
        FILE *fptr = fopen(argv[1],"r");
        while (fgets(inputString,512,fptr)) {
            char* next = strchr(inputString,'\n');
            printf("%s",inputString);
            *next = '\0';
            execFlag = processString(inputString, 
            parsedArgs, parsedArgsPiped); 

            if (execFlag == 1) 
                execArgs(parsedArgs); 
    
            if (execFlag == 2) 
                execArgsPiped(parsedArgs, parsedArgsPiped);
            
            printf("\n");
        }
        
    }
    // wait(NULL);
    return 0; 
}
