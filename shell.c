#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>

#define MAX_SIZE 1064



// cant be void and not return anything we need to return the pointer 
char *read_line(){
    int ch, i = 0;
    int size = MAX_SIZE;
    char *buffer = malloc(size * sizeof(char));
    if(!buffer){
        fprintf(stderr, "Allocation failed\n");
        exit(EXIT_FAILURE);
    }

    while((ch = getchar()) != '\n'){

        buffer[i++] = ch;


        if(i >= size){
            size += MAX_SIZE;
            buffer = realloc(buffer, size * sizeof(char));
            if(!buffer){
                fprintf(stderr, "Reallocation failed in read_line func\n");
                exit(EXIT_FAILURE);
            }
            //buffer[i++] = ch; dont need this
        }

    }
    buffer[i] = '\0';
    return buffer;



}

#define TOKEN_BUFSIZE 64
#define TOKEN_DELIM " \t\n\r\a"

// returns an array of pointers, points to an array of pointers which they point to portions of commands
char **split_line(char *line){
    
    int bufsize = TOKEN_BUFSIZE, i = 0;
    // **tokens is an array of pointers in which we will store the pointers to the command portions in, tokens = [token 1 pointer, token2 pointer ..., NULL]

    char **tokens = malloc(bufsize * sizeof(char *));
    char *token;

    if(!tokens){
        fprintf(stderr, "Allocation failed in split_line()\n");
        exit(EXIT_FAILURE);
    }
    
    token = strtok(line, TOKEN_DELIM);
    while(token != NULL){
        tokens[i++] = token;
        token = strtok(NULL, TOKEN_DELIM);
        
        if(i >= TOKEN_BUFSIZE){
            bufsize += TOKEN_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char *));
            
            if(!tokens){
                fprintf(stderr, "Reallocation failed in split_line()\n");
                exit(EXIT_FAILURE);
            }
        }
        

    }

    tokens[i] = NULL; // to end the array of pointers
    return tokens;

}

int shell_launch(char **args){
    pid_t pid, wpid;
    int status;

    pid = fork();

    if(pid == 0){
        // child process
        execvp(args[0], args);
    }
    else{
        // parent process
        // make loop continue until either the child process gets exited normally or gets killed with a signal
        
        do{ // WUNTRACED: return even if the child is stopped (not just exited)
            wpid = waitpid(pid, &status, WUNTRACED);
        }while(!WIFEXITED(status) && !WIFSIGNALED(status));

    }

    return 1;



}

int sh_cd(char **args);
int sh_help(char **args);
int sh_exit(char **args);
int sh_cwd(char **args);
int sh_dir(char **args);
int history(char **args);

// an array of string pointers , an array of pointers to char
char *builtin_str[] = {
    "cd",
    "help",
    "pwd",
    "ls",
    "history",
    "exit"
};

// function pointer allows you to indirectly call the function with the pointer 
// we use an array of pointers to functions because we have more than one function to point to with this func pointer, and the & is optional in this case
int (*builtin_func[])(char **) = {&sh_cd, &sh_help, &sh_cwd, &sh_dir, &history, &sh_exit};



int sh_num_built_ins(){
    return sizeof(builtin_str) / sizeof(char *);
}

void write_hist(char *args){
    // HOME  is a standard unit env variable for users home dir path
    char *home = getenv("HOME");
    // snprintf is like printf but writes to a string instead of stdout 
    char filepath[1024];
    snprintf(filepath, sizeof(filepath), "%s/shell_c/.history.txt", home);
    FILE *file_io = fopen(filepath, "a");

    if(!file_io){
        fprintf(stderr, "Error in write_hist()\n");
        exit(EXIT_FAILURE);
    }
    fprintf(file_io, "%s\n", args);

    fclose(file_io);

    
}


// we pass it an array of poiners
int shell_execute(char **args){
    int i;
    if(args[0] == NULL){
        // empty comm
        return 1;
    }

    for(i = 0; i < sh_num_built_ins(); i++){
        // if the provided arguement was equal to one of the builtin strings
        // chekcs if the provided arg is a built_in function , if so it returns it with the provided arg
        if(strcmp(args[0], builtin_str[i]) == 0){
            write_hist(args[0]);
            return(builtin_func[i])(args); 
            // this returns the function with the arg for example sh_cd(some_dir) -> cd dir
        }
       
    }
    // if it wasnt one of the built in functions it tries to execute it as an eternal program with shell_launch()
    // if it is one of the built in commands it does NOT call execvp in shell_launch() and use execvp, child process
    //return shell_launch(args);
    printf("Invalid command\n");
    return 1;
}

void shell_loop(){
    char buffer[255];
    // store the returned pointer from read_line() into *line and returned double ptr from split_line() into **args
    char *line;
    char **args;
    int status;

    do{
        getcwd(buffer, sizeof(buffer));
        printf("%s> ", buffer);
        line = read_line();
        // split line returns an array of pointers args = [portion 1 pointer, portion2 pointer ...., NULL]
        args = split_line(line);

        status = shell_execute(args);

        free(line);
        free(args);
    
    // keeps running until 0 is returned, status != 0
    }while(status);

    
}
int main(int argc, char **argv){


    shell_loop();





    return EXIT_SUCCESS;
}
// we pass it a pointer to other pointers that point to portions of the string we tokenized 
int sh_cd(char **args){
    //chdir(args[1]);
    if(args[1] == NULL){
        fprintf(stderr, "sh_cd: Expected arguement to cd into\n");

    }
    else if(chdir(args[1]) != 0){
        fprintf(stderr, "Invalid directory or does not exist.\n");
        return 1;
    }
    else{
        printf("Moved to %s \n", args[1]);
        printf("/%s>\n", args[1]);


    }
        
    
    return 1;
}

int sh_cwd(char **args){
    char buffer[1024];

    if(getcwd(buffer, sizeof(buffer)) != NULL){
        printf("The current dir is: %s\n", buffer);
    }
    else{
        perror("error happend in sh_cwd()\n");
        return 1;
    }
    return 1;
}

int sh_help(char **args){
    int i;
    printf("****************************************\n");
    printf("Aram's experimental shell\n");
    printf("The following are built in: \n");

    for(int i = 0; i < sh_num_built_ins(); i++){
        printf(" %s\n", builtin_str[i]);
    }

    return 1;


}

int sh_dir(char **args){
    // pointer to directory stream 
    DIR *d;

    // struct dirent is defined inside dirent.h lib 
    struct dirent *dir;

    d = opendir(".");

    if(d){
        while((dir = readdir(d)) != NULL){
            printf("%s\n", dir->d_name);
        }
        closedir(d);
    }
    
    return 1;

}

int history(char **args){
    char *home = getenv("HOME");
    char filepath[1024];
    snprintf(filepath, sizeof(filepath), "%s/shell_c/.history.txt", home);
    FILE *file_read = fopen(filepath, "r");
    char buffer[255];

    while((fgets(buffer, 255, file_read)) != NULL){
        
        printf("%s", buffer);
    }

    fclose(file_read);

    return 1;
}

int sh_exit(char **args){
    printf("Goodbye\n");
    return 0;
}