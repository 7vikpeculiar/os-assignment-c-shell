#include<stdio.h>
#include<pwd.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<unistd.h>
#include<limits.h>
#include<string.h>
#include<dirent.h>
#include<time.h>
#include<grp.h>
#include<signal.h>

#define max_size_parse_array 200
#define max_parsed_buffsize 10
#define max_pid_size 20
#define max_pid_path_name 100
#define max_path_length 1024
#define max_reminder_name 10
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"
#define ANSI_COLOR_RED   "\x1b[31m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"

char posix_delimit[]=" \t\r\n\v\f";
char* glob_input;

char* read_input();
char** parse_input(char*);
int run_executable_input(char**);
void bsh_looper();
int run_executable_input(char**);
int check_builtin(char**);
int run_builtins(char**);
int run_input(char**);
int run_pwd(char**);
int run_echo(char**);
int run_pinfo(char**);
int run_ls(char**);
int check_ls_flags(char**);
char* return_dir(char**);
void list(char*);
void list_all(char*);
void long_list(char*);
void long_list_all(char*);
char global_starting_dir[1024];
char tilda_modified_dir[1024];
char list_dir_arr[1024];
char* sys_name_for_prompt;
char* get_pwd_for_prompt;
char *user_name_for_prompt;


void sigint_handler(int signo) {
    printf("\n");
}

void run_remindme(char** parse_input)
{
  int exec_status;
  pid_t wpid;
  if(parse_input[1] == NULL){printf("remindme needs two args\n"); return;}
  if(parse_input[2] == NULL){printf("remindme needs two args\n"); return;}

  char** proper_command = malloc(sizeof(char*)*3);
  proper_command[0] = malloc(sizeof(char)*10);
  proper_command[1] = malloc(sizeof(char)*10);
  proper_command[2] = NULL;

  strcpy(proper_command[0],"sleep");
  strcpy(proper_command[1],parse_input[1]);
  pid_t pid1 = fork();
  if(pid1 == 0){
    signal(SIGINT, SIG_DFL);
    pid_t pid = fork();
    if(pid == 0){
      exec_status = execvp(proper_command[0],proper_command);
      if(exec_status == -1)
      {perror("Error:");}
      exit(EXIT_FAILURE);
      }
      else if(pid < 0){
        perror("Fork error");
      }else
      {
        do{
          wpid = waitpid(pid,&exec_status,WUNTRACED);
        }while(!(WIFEXITED(exec_status) || WIFSIGNALED(exec_status)));
      }
      printf("Reminder ::::  ");
      int iterer = 2;
      while(parse_input[iterer] != NULL){
        printf("%s ",parse_input[iterer]);
        iterer ++;
      }
      printf("\n");
    }
}
void mixer(int n)
{
  switch(n)
{
case 1:
   printf("Jan ");
   break;
case 2:
   printf("Feb ");
   break;
case 3:
   printf("Mar ");
   break;
case 4:
   printf("Apr ");
   break;
case 5:
   printf("May ");
   break;
case 6:
   printf("Jun ");
   break;
case 7:
   printf("Jul ");
   break;
case 8:
   printf("Aug ");
   break;
case 9:
   printf("Sep ");
   break;
case 10:
   printf("Oct ");
   break;
case 11:
   printf("Nov ");
   break;
case 12:
   printf("Dec ");
   break;
}}
void run_clock(char** parse_input)
{
  if(parse_input[1] == NULL){printf("clock needs two args\n"); return;}
  if(parse_input[3] == NULL){printf("clock needs two args\n"); return;}
  int tim = atoi(parse_input[2]);
  int dur = atoi(parse_input[4]);
  int i = 0;
  char path_rtc[] = "/proc/driver/rtc";
  char time_arr[30];
  char day_arr[30];
  int d,m,y;
  FILE * fp1;
  for(i = 0; i < dur/tim; i++)
  {
    fp1 = fopen(path_rtc,"r");
    if(fp1 != NULL){ // No errors
      fscanf(fp1,"%*s\t: %s\n%*s\t: %d-%d-%d",time_arr,&y,&m,&d);
      fclose(fp1);
      printf("%d ",d);
      mixer(m);
      printf("%d, -- %s\n",y, time_arr);
    }
    sleep(tim);
  }
}

void tildify()
{
  char temp_buff[1024];
  getcwd(temp_buff,1024);
  char* pointerer = strstr(temp_buff,global_starting_dir);
  int len = strlen(global_starting_dir);
  if(pointerer != NULL) {snprintf(tilda_modified_dir,1024,"~%s",&temp_buff[len]);}
  else {strcpy(tilda_modified_dir,temp_buff);}
  return;
}

int get_sys_name()
{
  sys_name_for_prompt = (char*)malloc(sizeof(char)*100);
  if(gethostname(sys_name_for_prompt,100) != -1)
  {return 0;}
  return 1;
}

int run_echo(char** parsed_string)
{
  int j;
  if (parsed_string[1] != NULL)
  {
    int i = 1;
    while(parsed_string[i] != NULL)
    {
      printf("%s ",parsed_string[i]);
      i++;
    }
  }
  printf("\n");
}

int run_cd(char** parsed_string)
{
  if (parsed_string[1] == NULL)
  {
    if(chdir(global_starting_dir) != 0)
    {      perror("Error: ");    }
  }
  else if(strcmp(parsed_string[1],"&") == 0){   if(chdir(global_starting_dir) != 0){ perror("Error: ");}} // if cmd is cd &
  else if(parsed_string[2] == NULL || strcmp(parsed_string[2],"&") == 0)
  {
    if(parsed_string[1][0] == '~')
    {
      char temp[200];
      snprintf(temp,200,"%s%s",global_starting_dir,&parsed_string[1][1]);
      if(chdir(temp) == -1) printf(ANSI_COLOR_RED "cd: no such file or directory: %s\n" ANSI_COLOR_RESET,parsed_string[1]);
    }
    else if(chdir(parsed_string[1]) == -1)
    {
        printf(ANSI_COLOR_RED "cd: no such file or directory: %s\n" ANSI_COLOR_RESET,parsed_string[1]);
    }
  }
  else
  {
    printf(ANSI_COLOR_RED "cd: string not in pwd: %s\n" ANSI_COLOR_RESET ,parsed_string[1]);
  }
}

int main()
{
    /*if(chdir(getenv("HOME")) != 0)
    {
      perror("Error: ");
    }
    chdir("./Downloads/Sem 3-1/OS/Code/Assignment 2/");*/
    signal(SIGINT, sigint_handler);
    printf(ANSI_COLOR_YELLOW "--- Starting buggi_shell --\n" ANSI_COLOR_RESET);
    user_name_for_prompt = getenv("LOGNAME");
    if(user_name_for_prompt == NULL){printf(ANSI_COLOR_RED "Couldn't access user name\n" ANSI_COLOR_RESET);}
    if(getcwd(global_starting_dir,1024) == NULL) printf(ANSI_COLOR_RED "Couldn't get CWD\n" ANSI_COLOR_RESET);
    get_sys_name();
    bsh_looper();
    return 0;
}

void bsh_looper()
{

    int flag = 1;
    while(flag == 1)
    {
      tildify();
      printf(ANSI_COLOR_CYAN "<%s|buggi_shell|%s|%s>" ANSI_COLOR_RESET,user_name_for_prompt,sys_name_for_prompt,tilda_modified_dir);
      glob_input = read_input();
      char* temp_input =  malloc(sizeof(char)*1024);
      strcpy(temp_input,glob_input);
      char** parsed_line = parse_input(temp_input);
      if(parsed_line[0] == NULL)
      { //checking if no input is given
        continue;
      }
      if(strcmp(parsed_line[0],"exit") == 0)
      { printf(ANSI_COLOR_YELLOW "--- Exiting buggi_shell --\n" ANSI_COLOR_RESET);
          break;}
      //flag = run_executable_input(parsed_line);
      run_input(parsed_line);
      //call_getcwd();
      //free(input);
      //free(parsed_line); // getline is being freed -> man page suggestion
                         // freeing the set of strings, obtained from parsing
      //flag = 1;
    }
}

// GETS SHELL INPUT
char* read_input()
{
  char *buffer = NULL;
  ssize_t bufsize = 0;
  // this ensures both sie and pointer are allocated by getline
  getline(&buffer, &bufsize, stdin);
  return buffer;
}

//PARSES THE INPUTw
char** parse_input(char* user_input)
{
  //strtok doesnot thread it seems
  int size_parse_array = 50;
  char **parse_array = malloc(size_parse_array*sizeof(char*));
  char *saveptr;
  char * piece;
  int point = 0;
  piece = strtok_r(user_input,posix_delimit,&saveptr);
  // Piece contains the first piece
  while(piece != NULL) // Until end of the parsed parse_array
  {
    if(size_parse_array == point) // No space is left
    {
      size_parse_array += max_parsed_buffsize;
      parse_array = realloc(parse_array, sizeof(char*)*size_parse_array);
      if(parse_array== NULL)
      { // Error checking if realloc can't happen
        fprintf(stderr, "Error allocating space\n");
        exit(EXIT_FAILURE);
      }
    }
    parse_array[point] = piece;
    point++;
    piece = strtok_r(NULL,posix_delimit,&saveptr); //NULL => continue parsing the same string
  }
    return parse_array;
}

int run_executable_input(char** processed_inp)
{
  //pid_t type to store pids specifically
  int exec_status;
  int wpid;
  int token_iterer = 0;
  char* last_val;
  while(processed_inp[token_iterer] != NULL)
  {
    last_val = processed_inp[token_iterer];
    token_iterer += 1;
  }
  if(strcmp(last_val,"&") == 0)
    {
      //signal (SIGCHLD, proc_exit);
      processed_inp[token_iterer -1] = NULL;
      pid_t pid1 = fork();
      if(pid1 == 0){
        signal(SIGINT, SIG_DFL);
        pid_t pid = fork();
        if(pid == 0){
          exec_status = execvp(processed_inp[0],processed_inp);
          if(exec_status == -1)
          {perror("Error:");}
          exit(EXIT_FAILURE);
          }
          else if(pid < 0){
            perror("Fork error");
          }else
          {
            do{
              wpid = waitpid(pid,&exec_status,WUNTRACED);
            }while(!(WIFEXITED(exec_status) || WIFSIGNALED(exec_status)));
          }
          printf("pid %d has exited normally\n",wpid);
        }
      }
  else{
    pid_t pid = fork();
    if(pid == 0){
    signal(SIGINT, SIG_DFL);
    exec_status = execvp(processed_inp[0],processed_inp);
    if(exec_status == -1)
    {perror("Error:");}
    exit(EXIT_FAILURE);
  }
  else if(pid < 0){
      perror("Fork error");
  }else
  {
    do{
      wpid = waitpid(pid,&exec_status,WUNTRACED);
    }while(!(WIFEXITED(exec_status) || WIFSIGNALED(exec_status)));
  }
  }
  return 1;
}

int check_builtin(char** command)
{
    if(strcmp(command[0],"echo") == 0)
    {
      run_echo(command);
      return 1;
    }
    else if(strcmp(command[0],"pwd") == 0)
    {
      run_pwd(command);
      return 1;
    }else if(strcmp(command[0],"cd") == 0 )
    {
        run_cd(command);
        return 1;
    }
    else if(strcmp(command[0],"ls") == 0)
    {
      run_ls(command);
      return 1;
    }
    else if(strcmp(command[0],"pinfo") == 0)
    {
      run_pinfo(command);
      return 1;
    }
    else if(strcmp(command[0],"remindme") == 0)
    {
      run_remindme(command);
      return 1;
    }
    else if(strcmp(command[0],"clock") == 0)
    {
      run_clock(command);
      return 1;
    }
    return -1000;
}

int run_input(char ** parsed_input)
{
  if(check_builtin(parsed_input) == -1000)
  {run_executable_input(parsed_input);}
  return 0;
}

int run_pwd(char** parsed_string)
{
  if(parsed_string[1] != NULL)
  {
    printf("pwd: too many arguments\n"); return 0;
  }
  size_t bufsize = 1024;
  char *buffer = malloc(sizeof(char)*bufsize);
  //char* buffer = NULL;
  getcwd(buffer, bufsize);
  //return buffer;
  printf("%s\n",buffer);
  free(buffer);
  return 1;
}

int run_pinfo(char** parsed_string)
{
  char* pid_string = malloc(sizeof(char)*max_pid_size);
  int pid;
  char pstate;
  long long int vm_size;
  FILE *fp1,*fp2;
  char* path_stat = malloc(sizeof(char)*max_pid_path_name);
  char* path_statm = malloc(sizeof(char)*max_pid_path_name);
  char* path_exe = malloc(sizeof(char)*max_pid_path_name);
  char* exe_link = malloc(sizeof(char)*max_path_length);
  if(parsed_string[1] == NULL)
  {
    pid = getpid();
    snprintf(pid_string,max_pid_size,"%d",pid);
  }
  else if(parsed_string[2] != NULL)
  { // Checking if more than one arg is given
    printf("pinfo command takes one argument at a time\n");
    return 1;
  }
  else
  { // Has one argument
    strcpy(pid_string,parsed_string[1]);
  }
  snprintf(path_stat,max_pid_path_name,"/proc/%s/stat",pid_string);
  snprintf(path_statm,max_pid_path_name,"/proc/%s/statm",pid_string);
  snprintf(path_exe,max_pid_path_name,"/proc/%s/exe",pid_string);

  fp1 = fopen(path_stat,"r");
  fp2 = fopen(path_statm, "r");

  if(fp1 != NULL){ // No errors
    printf("pid -- %s\n",pid_string);
    fscanf(fp1,"%*s %*s %c",&pstate);
    printf("Process Status -- %c\n",pstate);
    fclose(fp1);
  }
  else{
      printf("No process with proc Id %s exists\n", pid_string);
      return 1;
  }

  if(fp2 != NULL){ // No errors
    fscanf(fp2,"%lld",&vm_size);
    printf("Memory -- %lld\n", vm_size);
    fclose(fp2);
  }
  if(readlink(path_exe,exe_link,max_path_length) != -1)
  {
    printf("Executable path -- %s",exe_link);
  }
  printf("\n");

  free(pid_string);
  free(path_stat);
  free(path_statm);
  free(path_exe);
}

int check_ls_flags(char** parsed_string)
{
  int token_iterer = 0;
  int a = 00; //la
  while(parsed_string[token_iterer] != NULL)
  {
    if(strcmp(parsed_string[token_iterer],"-a") == 0)   a += 1;

    if(strcmp(parsed_string[token_iterer],"-l") == 0)  a += 10;

    if(
      ( strcmp(parsed_string[token_iterer],"-la") == 0)
        || (strcmp(parsed_string[token_iterer],"-al") == 0)
      ) a += 11;
    token_iterer ++;
  }
  return a;
}

char* return_dir(char** parsed_string)
{
  char* fname;
  size_t bufsize = 1024;
  int token_iterer = 0;
  int a = 0;
  while(parsed_string[token_iterer] != NULL)
  {
    if(!
      ((strcmp(parsed_string[token_iterer],"-a") == 0) ||
      (strcmp(parsed_string[token_iterer],"-l") == 0) ||
      (strcmp(parsed_string[token_iterer],"-la") == 0) ||
      (strcmp(parsed_string[token_iterer],"-al") == 0) ||
      (strcmp(parsed_string[token_iterer],"ls") == 0) )
      ){a += 1; fname = parsed_string[token_iterer];}
      token_iterer ++;
  }
  if(a == 0 || (strcmp(fname,"&") == 0))
  {
    if(getcwd(fname, bufsize) == NULL)
    {
      printf(ANSI_COLOR_RED "Couldnt access files in the CWD\n" ANSI_COLOR_RESET);
    };
  }
  if(fname[0] == '~')
  {
    char temp[500];
    snprintf(temp,500,"%s%s",global_starting_dir, &fname[1]);
    strcpy(fname,temp);
  }
  return fname;
}

void list(char *path)
{
   struct dirent *entry;
   //printf("%s\n", path);
   DIR *dir = opendir(path);
   if (dir == NULL) {
      printf(ANSI_COLOR_RED "Couldn't access directory\n" ANSI_COLOR_RESET);
      return;
   }
   while ((entry = readdir(dir)) != NULL) {
      //if(!(strcmp(entry->d_name,".") == 0 || strcmp(entry->d_name,"..") == 0))
      if(entry->d_name[0] != '.'){
      printf("%s\t",entry->d_name);
      }
   }
   printf("\n");
   closedir(dir);
}

void list_all(char *path)
{
   struct dirent *entry;
   printf("%s\n", path);
   DIR *dir = opendir(path);
   if (dir == NULL) {
      printf(ANSI_COLOR_RED "Couldn't access directory\n" ANSI_COLOR_RESET);
      return;
   }
   while ((entry = readdir(dir)) != NULL) {
      printf("%s\t",entry->d_name);
   }
   printf("\n");
   closedir(dir);
}

void long_list(char* path)
{
  struct dirent *entry;
  struct stat fileStat;
  struct passwd* pws;
  struct group * grp;
  time_t result;
  char buff[30];

  DIR *dir = opendir(path);
  if (dir == NULL) {
     printf(ANSI_COLOR_RED "Couldn't access directory %s\n" ANSI_COLOR_RESET, path);
     return;
  }
  while ((entry = readdir(dir)) != NULL) {
     if(entry->d_name[0] == '.') continue;

     // To ensure hidden files are not printed
     if(stat(entry->d_name,&fileStat) == -1)
     {
       printf(ANSI_COLOR_RED "Couldn't access directory/file %s\n" ANSI_COLOR_RESET, entry->d_name);
     };
     (S_ISDIR(fileStat.st_mode)) ? printf("d") : printf("-");
     (fileStat.st_mode & S_IRUSR) ? printf("r") : printf("-");
     (fileStat.st_mode & S_IWUSR) ? printf("w") : printf("-");
     (fileStat.st_mode & S_IXUSR) ? printf("x") : printf("-");
     (fileStat.st_mode & S_IRGRP) ? printf("r") : printf("-");
     (fileStat.st_mode & S_IWGRP) ? printf("w") : printf("-");
     (fileStat.st_mode & S_IXGRP) ? printf("x") : printf("-");
     (fileStat.st_mode & S_IROTH) ? printf("r") : printf("-");
     (fileStat.st_mode & S_IWOTH) ? printf("w") : printf("-");
     (fileStat.st_mode & S_IXOTH) ? printf("x") : printf("-");
     printf(" %lu",fileStat.st_nlink);
     pws = getpwuid(fileStat.st_uid);
     (pws != NULL) ? printf("%8s", pws->pw_name) : printf("%8d", fileStat.st_uid);
     grp = getgrgid(fileStat.st_gid);
     (grp != NULL) ? printf("%8s", grp->gr_name): printf("%8d", fileStat.st_gid);
     printf("  %5ld", fileStat.st_size);
     result = fileStat.st_mtime;
     strftime(buff, 30, "%b %d %H:%M", localtime(&result));
     printf("  %s ",buff);
     //printf(" %lu",fileStat.st_mtime);
     printf("  %s\n",entry->d_name);

      }
  closedir(dir);
  return;
}

void long_list_all(char* path)
{
  struct dirent *entry;
  struct stat fileStat;
  struct passwd* pws;
  struct group * grp;
  time_t result;
  char buff[30];

  //printf("%s\n", path);
  DIR *dir = opendir(path);
  if (dir == NULL) {
     printf(ANSI_COLOR_RED "Couldn't access directory %s\n" ANSI_COLOR_RESET, path);
     return;
  }
  while ((entry = readdir(dir)) != NULL) {
     if(stat(entry->d_name,&fileStat) == -1)
     {
       printf(ANSI_COLOR_RED "Couldn't access directory/file %s\n" ANSI_COLOR_RESET, entry->d_name);
     };
     (S_ISDIR(fileStat.st_mode)) ? printf("d") : printf("-");
     (fileStat.st_mode & S_IRUSR) ? printf("r") : printf("-");
     (fileStat.st_mode & S_IWUSR) ? printf("w") : printf("-");
     (fileStat.st_mode & S_IXUSR) ? printf("x") : printf("-");
     (fileStat.st_mode & S_IRGRP) ? printf("r") : printf("-");
     (fileStat.st_mode & S_IWGRP) ? printf("w") : printf("-");
     (fileStat.st_mode & S_IXGRP) ? printf("x") : printf("-");
     (fileStat.st_mode & S_IROTH) ? printf("r") : printf("-");
     (fileStat.st_mode & S_IWOTH) ? printf("w") : printf("-");
     (fileStat.st_mode & S_IXOTH) ? printf("x") : printf("-");
     printf(" %lu",fileStat.st_nlink);
     pws = getpwuid(fileStat.st_uid);
     (pws != NULL) ? printf("%8s", pws->pw_name) : printf("%8d", fileStat.st_uid);
     grp = getgrgid(fileStat.st_gid);
     (grp != NULL) ? printf("%8s", grp->gr_name): printf("%8d", fileStat.st_gid);
     printf("  %5ld", fileStat.st_size);
     result = fileStat.st_mtime;
     strftime(buff, 20, "%b %d %H:%M", localtime(&result));
     printf("  %s ",buff);
     //printf(" %lu",fileStat.st_mtime);
     printf("  %s\n",entry->d_name);

      }
  closedir(dir);
  return;
}

int run_ls(char** parsed_string)
{
  char* fname;
  size_t bufsize = 1024;
  char* temp_input = malloc(sizeof(char)*1024);
  strcpy(temp_input,glob_input);
  char** temp_parsed_string = parse_input(temp_input);
  int flaggos = check_ls_flags(temp_parsed_string);
  fname = return_dir(temp_parsed_string);
  //list(fname);
  printf("%d\n",flaggos);
  if(flaggos == 0) list(fname);
  if(flaggos == 1) list_all(fname);
  if(flaggos == 10) long_list(fname);
  if(flaggos == 11) long_list_all(fname);
  //free(fname);
  free(temp_parsed_string);
  return 1;
}
