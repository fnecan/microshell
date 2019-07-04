#define _GNU_SOURCE
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <dirent.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <limits.h>
#include <utime.h>
#include <fcntl.h>
#include <time.h>

#define MAX_DIR_BUFFER 128
#define MAX_LINE_BUFFER 1024
#define MAX_LINE_ARGS_COUNT 64
#define MAX_ARG_CHAR_COUNT 32

typedef char* String;
typedef char** StringArray;

#define RED     "\x1b[31m"
#define BLUE    "\x1b[34m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"

String username = "Default";
String homePath = "/";
char currentDirectory[MAX_DIR_BUFFER];
char lineBuffer[MAX_LINE_BUFFER];

void getCurrentDir(){
   char path[MAX_DIR_BUFFER];
   if (getcwd(path, sizeof(path)) != NULL) {
       strcpy(currentDirectory,path);
   } else {
       perror("shell");
   }   
}

void init(){
    getCurrentDir();
    struct passwd * user = getpwuid(getuid()); 
    homePath = user->pw_dir;
    username = user->pw_name;
    if(username == NULL){
        username = "def";
    }
}

StringArray parseLine(String line){
    
    StringArray allocatedArray = (StringArray)malloc(MAX_LINE_ARGS_COUNT * sizeof(char*) * MAX_ARG_CHAR_COUNT);
    String limiter = " \n";
    int i = 0;
    String arg = strtok(line, limiter);
    allocatedArray[0] = arg;
    for(;i < MAX_LINE_ARGS_COUNT; i++){        
        allocatedArray[i+1] = NULL;
        arg = strtok(NULL, limiter);
        allocatedArray[i+1] = arg;
    }   

    return allocatedArray;
}
/*
void _devPrintArray(StringArray newArray){
    int i = 0;
    for(i;i<MAX_LINE_ARGS_COUNT;i++){
        if(newArray[i] != NULL){
            printf("%d: %s\n",i,newArray[i]);
        }
    }
}
*/
void startProcess(String command, StringArray arguments)
{
  pid_t pid;
  int status;

  pid = fork();
  if (pid == 0) {
    if (execvp(command, arguments) == -1)  perror("shell");
    exit(1);
  } else {
      waitpid(pid, &status, WUNTRACED);
  }
}

void bi_cd(StringArray args){
  if (args[1] == NULL) {
      printf("Provide proper path");
  } else {
    String path = args[1];
    if(args[1][0] == '~'){
      String initialPath = args[1];
      initialPath++;
      const String initial = initialPath;
      const String home = homePath;
      path = (char *)malloc(strlen(initial)+strlen(home)+1);
   
    strcat(path,homePath);
    strcat(path,initialPath);
    }
    if (chdir(path) != 0) {
      perror("shell");
    }
  }    
    getCurrentDir();
}

void bi_help(){
    printf("\nMicroshell created by Artur Kmiećkowiak\n");
    printf("Available built-in commands: ls, touch, pwd\n");
    printf("Flags adding demonstration ls -a , touch -c\n");
    printf("type exit to quit\n\n");
}

void bi_exit(){
    exit(1);
}

StringArray getArguments(StringArray initialArguments){
    StringArray allocatedArray = (StringArray)malloc(MAX_LINE_ARGS_COUNT);
    int arrayCounter = 0;
    int i = 1;
    for(;i < MAX_LINE_ARGS_COUNT ; i++){
        if(initialArguments[i] == NULL) break;
        if(initialArguments[i][0] != 45){
          allocatedArray[arrayCounter] = initialArguments[i];
          arrayCounter++;
        }
        if(arrayCounter == 0 && i == MAX_LINE_ARGS_COUNT - 1){
          allocatedArray[0] = NULL;
        }
    }
    return allocatedArray;
}

int lookForFlag(StringArray flags, String flag){
    int i = 0;
    for(; i < 64 ; i++){
        if(flags[i] == NULL){
            return -1;
        }else if(strcmp(flags[i],flag) == 0){
            return i;
        }
    }
    return -1;
}

StringArray getFlags(StringArray initialArguments){
    StringArray allocatedArray = (StringArray)malloc(MAX_LINE_ARGS_COUNT);
    int arrayCounter = 0;
    int i = 1;
    for(;i < MAX_LINE_ARGS_COUNT ; i++){
        if(initialArguments[i] == NULL) break;
        if(initialArguments[i][0] == 45){
          allocatedArray[arrayCounter] = initialArguments[i];
          arrayCounter++;
        }
        if(arrayCounter == 0 && i == MAX_LINE_ARGS_COUNT - 1){
          allocatedArray[0] = NULL;
        }
    }
    return allocatedArray;
}

void bi_touch(StringArray arguments){
    StringArray args = getArguments(arguments);
    StringArray flags = getFlags(arguments);
    int mode = lookForFlag(flags,"-c");
    if(args[0] == NULL){
        printf("Path for file needed");
        return;
    }
    int i = 0;
    for(;i<64;i++){
        if(args[i] == NULL) break;
        if(access(args[i], F_OK) == -1 ){
            if(mode == -1){
                int fd = creat(args[i], S_IXUSR | S_IRWXU );
                if(fd < 0){
                    perror("shell");
                }
            }
        }else{
            String argument = args[i];
            struct utimbuf currentTime;
            time_t currTimestamp;
            time(&currTimestamp);
            currentTime.actime = currTimestamp;
            currentTime.modtime = currTimestamp;
            if(utime(argument, &currentTime) < 0) perror("Unable to set time");
        }    
    }
}

void bi_ls(StringArray arguments){
  StringArray args = getArguments(arguments);
  StringArray flags = getFlags(arguments);
  String dirPath = ".";
  int mode = lookForFlag(flags,"-a");
  if(args[0] != NULL) dirPath = args[0];
    
  DIR *openedDir;
  openedDir = opendir(dirPath);
  if(!openedDir){
      perror("shell");
      return;
  }
  struct dirent * dirContent;
  while(dirContent){
    dirContent = readdir(openedDir);
      if (dirContent)
      {
        if(dirContent->d_type == DT_DIR){
            if(mode != -1){
                printf(CYAN"%s"RESET"\t", dirContent->d_name);
            }else if(mode == -1){
                if(strcmp(dirContent->d_name,"..") != 0 && strcmp(dirContent->d_name,".") != 0){
                    printf(CYAN"%s"RESET"\t", dirContent->d_name);
                }
            }
        }else{
            printf("%s \t",dirContent->d_name);
        }
      }else{
          printf("\n");
      }
  }
    closedir(openedDir);
}

void bi_pwd(){
    printf("%s\n", currentDirectory);
}

void verifyMethod(String methodName, StringArray arguments){

if (strcmp(methodName, "cd") == 0) 
{
  bi_cd(arguments);
} 
else if (strcmp(methodName, "help") == 0)
{
  bi_help();
}
else if (strcmp(methodName, "exit") == 0)
{
  bi_exit();
}
else if (strcmp(methodName, "ls") == 0)
{
  bi_ls(arguments);
}
else if (strcmp(methodName, "touch") == 0){
  bi_touch(arguments);
}
else if (strcmp(methodName, "pwd") == 0){
  bi_pwd();
}
else
{
    startProcess(methodName, arguments);
}

}

void loop(){
    printf(RED"%s"RESET BLUE"%s"RESET ">",username, currentDirectory);
        String line = fgets(lineBuffer,MAX_LINE_BUFFER,stdin); 
        StringArray parsedArray = parseLine(line);
        String command = parsedArray[0];
        verifyMethod(command, parsedArray);
}

int main(){
    init();
    while(1){
        loop();
    }
}