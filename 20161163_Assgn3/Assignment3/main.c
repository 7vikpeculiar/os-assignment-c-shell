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
#include<fcntl.h>

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
#define ANSI_COLOR_GREEN   "\x1b[32m"

char posix_delimit[]=" \t\r\n\v\f";
char* glob_input;

char* read_input();
char** parse_input_string(char*);
int run_executable_input(char**);
void bsh_looper();
int run_executable_input(char**);
int check_builtin(char**);
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
void run_setenv(char**);
void run_unsetenv(char**);
void run_printenv(char**);
void run_kjob(char**);
void run_jobs(char**);
void run_fg(char**);
void run_bg(char**);
void run_overkill(char**);
int redirectify();
int close_redirectify();
int run_pipe();

char tilda_modified_dir[1024];
char global_starting_dir[1024];
char list_dir_arr[1024];
char* sys_name_for_prompt;
char* get_pwd_for_prompt;
char *user_name_for_prompt;
char** parsed_line;

int job_arr[100];
char job_desc[100][20];
int job_pntr = 0;
int glob_stdin_copy;
int glob_stdout_copy;
char* inp_redirect_arr = NULL;
char* out_redirect_arr = NULL;
char* append_redirect_arr = NULL;
int global_pipe_count;
char *** list_of_commands;

typedef struct procInfo
{
	  int proc_id;
	  char* instr;
		struct procInfo* next;
}procInfo;

procInfo* glob_Jobarr = NULL;

void insertProc(int procid,char* str)
{
	  procInfo* piece = (procInfo*) malloc(sizeof(procInfo));
		piece->proc_id = procid;
		piece->instr = malloc(sizeof(char)*(strlen(str)+1));
		strcpy(piece->instr,str);
		if(glob_Jobarr == NULL)
		{
				glob_Jobarr = piece;
				piece->next = NULL;
		}
		else
		{
			piece->next = glob_Jobarr;
			glob_Jobarr = piece;
		}
		return;
}

int delProc(int procid)
{
		if(glob_Jobarr == NULL)
		{ return -1;}
		procInfo* curr;
		procInfo* prev;
		int iter = 0;
		for(curr = glob_Jobarr, prev = NULL; curr != NULL; prev = curr, curr = curr->next, iter++)
		{
				if(curr->proc_id == procid)
				{
					if(prev == NULL)
					{
						glob_Jobarr = curr->next;
					}else{
						prev->next = curr->next;
					}
					free(curr);
					return iter;
				}
		}
		return -2;
}

procInfo* RemovefirstProcs()
{
  procInfo* old;
  int iter;
  if(glob_Jobarr != NULL)
  {
    old = glob_Jobarr;
    glob_Jobarr = glob_Jobarr->next;
    return old;
  }
  return NULL;
}

int printAllJobs()
{
	int iter = 0;
  char pstate;
  FILE * fp1;
  char* path_stat = malloc(sizeof(char)*max_pid_path_name);
	procInfo* start = glob_Jobarr;
	while(start != NULL)
	 {
     iter++;
  //   snprintf(path_stat,max_pid_path_name,"/proc/%d/stat",start->proc_id);
  //   fp1 = fopen(path_stat,"r");
  //   if(fp1 != NULL){ // No errors
  //     fscanf(fp1,"%*s %*s %c",&pstate);
  //     fclose(fp1);
  //   }
		printf(ANSI_COLOR_GREEN "[%d] -- [State - %c] -- %s [%d] \n" ANSI_COLOR_RESET,iter,pstate,start->instr, start->proc_id);
		start = start->next;
	}
  free(path_stat);
	return iter;
}

int getJob(int num)
{
		int iter = 1;
		procInfo* start = glob_Jobarr;
		if(glob_Jobarr == NULL)
		{
			return -1;
		}
		while(start != NULL)
		{
			if(iter == num)
			{
				return start->proc_id;
			}
			start = start->next;
			iter++;
		}
		return -2;
}

void sigint_handler(int signo) {
    //printf("\n");
}

void sigtstp_handler(int sig)
{
  int pid = getpid();
  if(kill(pid, SIGTSTP) == -1)
  {
    perror("kill : error \n");
  }
  insertProc(pid,"(noinfo)");
  return;
}

void run_overkill(char** parsed_input)
{
  procInfo* tmp = RemovefirstProcs();
  if(tmp == NULL)
  {
    printf(ANSI_COLOR_YELLOW"No jobs to be killed \n"ANSI_COLOR_RESET);
    return;
  }
  while(tmp != NULL)
  {
    if(kill(tmp->proc_id, SIGKILL) == -1)
    {printf(ANSI_COLOR_RED"Couldnt kill process %d %s \n"ANSI_COLOR_RESET, tmp->proc_id, tmp->instr);}
    else{printf(ANSI_COLOR_GREEN"Killing job %d %s ...\n"ANSI_COLOR_RESET, tmp->proc_id, tmp->instr);}
    tmp = RemovefirstProcs();
  }
  return;
}

void run_fg(char** parse_input)
{
  int fg_status,wpid;
  if(parse_input[1] == NULL){perror("fg needs one arguments");}
  else if(parse_input[3] != NULL){perror("fg needs one argument only");}
  else{
    int fake_proc_id = atoi(parse_input[1]);
    int actual_proc_id = getJob(fake_proc_id);
    if(actual_proc_id == -1 || actual_proc_id == -2){ printf("Invalid job\n"); return;}
    //signal()

    if(kill(actual_proc_id,SIGCONT) == -1)
     {
			 printf("API is %d--\n",actual_proc_id);
       perror("fg error");
     }
     else
     {
			 do {
		wpid = waitpid(actual_proc_id, &fg_status, WUNTRACED);
	} while (!WIFEXITED(fg_status) && !WIFSIGNALED(fg_status));
}}
   return;
}

void run_bg(char** parse_input)
{
  int bg_status;
  if(parse_input[1] == NULL){perror("bg needs one arguments");}
  else if(parse_input[3] != NULL){perror("bg needs one argument only");}
  else{
    int fake_proc_id = atoi(parse_input[1]);
    int actual_proc_id = getJob(fake_proc_id);
    if(actual_proc_id == -1 || actual_proc_id == -2){ printf("Invalid job\n"); return;}
    if(kill(actual_proc_id,SIGCONT) == -1)
     {
			 delProc(actual_proc_id);
       perror("bg error");
     }
     else
     {
      pid_t pid=waitpid(actual_proc_id,&bg_status,WUNTRACED);}
   }
   return;

}

void run_jobs(char** parse_input)
{
  if(parse_input[1] != NULL){perror("jobs needs no arguments\n");}
  else{
    if(printAllJobs() == 0)
    {
        printf(ANSI_COLOR_YELLOW "no jobs to be displayed \n" ANSI_COLOR_RESET);
    }
  }
  return;
}

void run_kjob(char** parse_input)
{
  if(parse_input[1] == NULL){perror("kjob needs atleast two arguments\n");}
  else if(parse_input[2] == NULL){perror("kjob needs atleast two arguments\n");}
  else if(parse_input[3] == NULL)
  {
    int fake_proc_id = atoi(parse_input[1]);
    int signal_num = atoi(parse_input[2]);
		int procid = getJob(fake_proc_id);
    if(procid == -1){ printf(ANSI_COLOR_RED "Invalid job\n" ANSI_COLOR_RESET); return;}
    if(kill(procid,signal_num) == -1)
     {
       perror("kjob encountered an error");
			 delProc(procid);
       return;
     }
     if(signal_num == 9)
     {delProc(procid);}
  }
  else{perror("kjob needs maximum of two argument\n");}
  return;
}

void run_setenv(char** parse_input)
{
  if(parse_input[1] == NULL){perror("setenv needs atleast one argument\n");}
  else if(parse_input[2] == NULL)
  {
     if(setenv(parse_input[1],"",1) == -1)
     {
       perror("Couldnt set environment variable");
     }
  }
  else if(parse_input[3] == NULL) //
  {
    if(setenv(parse_input[1],parse_input[2],1) == -1) {
       perror("Couldnt set environment variable");
     }
  }
  else{perror("setenv needs maximum of two argument\n");}
  return;
}

void run_unsetenv(char** parse_input)
{
  if(parse_input[1] == NULL){perror("unsetenv needs atleast one argument\n");}
  else if(parse_input[2] == NULL)
  {
     if(unsetenv(parse_input[1]) == -1)
     {
       perror("Couldnt set environment variable");
     }
  }
  else{perror("setenv needs maximum of one argument\n");}
  return;
}

void run_printenv(char** parse_input)
{
  printf("%s \n",getenv(parse_input[1]));
  return;
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
  return 0;
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
		signal(SIGKILL,SIG_IGN);
    signal(SIGINT, SIG_IGN);
		signal(SIGTSTP,SIG_IGN);
		signal(SIGCHLD,SIG_IGN);
    printf(ANSI_COLOR_YELLOW "--- Starting buggi_shell --\n" ANSI_COLOR_RESET);
    user_name_for_prompt = getenv("LOGNAME");
    if(user_name_for_prompt == NULL){printf(ANSI_COLOR_RED "Couldn't access user name\n" ANSI_COLOR_RESET);}
    if(getcwd(global_starting_dir,1024) == NULL) printf(ANSI_COLOR_RED "Couldn't get CWD\n" ANSI_COLOR_RESET);
    get_sys_name();
    glob_stdin_copy = dup(0);
    glob_stdin_copy = dup(1);
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
      parsed_line = parse_input_string(temp_input);

      if(parsed_line[0] == NULL)
      { //checking if no input is given
        continue;
      }
      if(strcmp(parsed_line[0],"quit") == 0 || strcmp(parsed_line[0],"exit") == 0)
      { printf(ANSI_COLOR_YELLOW "--- Exiting buggi_shell --\n" ANSI_COLOR_RESET);
          break;}
      //flag = run_executable_input(parsed_line);
      redirectify();
      int run_p_output = run_pipe();
      if(run_p_output != 0)
      {
        printf(ANSI_COLOR_YELLOW "-- Pipes evaluated -- %d\n" ANSI_COLOR_RESET,run_p_output);
        close_redirectify();
        continue;
      }
      run_input(parsed_line);
      close_redirectify();
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

//PARSES THE INPUT
char** parse_input_string(char* user_input)
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
  if(strcmp(last_val,"&") == 0) // bg_proc
    {
      char* temp_input = malloc(sizeof(char)*200);
      strcpy(temp_input,glob_input);
      temp_input[strlen(temp_input) -2] = '\0';
      char** processed_inp = parse_input_string(temp_input);
      pid_t pid1 = fork();
      signal(SIGINT, SIG_DFL);
      if(pid1 == 0){
				signal(SIGINT, SIG_DFL);
				signal(SIGTSTP,SIG_DFL);
          exec_status = execvp(processed_inp[0],processed_inp);
          if(exec_status == -1)
          {perror("Error:");}
          exit(EXIT_FAILURE);
          }
          else if(pid1 < 0){
            perror("Fork error");
          }
          else if(pid1 != 0)
          {
            temp_input = malloc(sizeof(char)*100);
            strcpy(temp_input,glob_input);
            temp_input[strlen(temp_input) -2] = '\0';
            insertProc(pid1, temp_input);
            printf(ANSI_COLOR_YELLOW "Background process stated\n" ANSI_COLOR_RESET);
          }
      }
  else{ // foregs process
    pid_t pid = fork();
    if(pid == 0){
			signal(SIGTSTP,SIG_DFL);
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
		wpid = waitpid(pid,&exec_status,WUNTRACED);
		if(WIFSTOPPED(exec_status))
		{
		if(WSTOPSIG(exec_status) == 18 || WSTOPSIG(exec_status) == 20 || WSTOPSIG(exec_status) == 24)
		{
			char* tmp = malloc(sizeof(char)*100);
			strcpy(tmp,glob_input);
			insertProc(getpid(), tmp);
		} //ifend
	} //ifend

}}
  return 1;
}

int check_builtin(char** command)
{
    if(strcmp(command[0],"echo") == 0)
    {
      //run_echo(command);
      //return 1;
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
      //run_ls(command);
      //return 1;
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
    else if(strcmp(command[0],"setenv") == 0)
    {
      run_setenv(command);
      return 1;
    }
    else if(strcmp(command[0],"unsetenv") == 0)
    {
      run_unsetenv(command);
      return 1;
    }
    else if(strcmp(command[0],"printenv") == 0)
    {
      run_printenv(command);
      return 1;
    }
    else if(strcmp(command[0],"kjob") == 0)
    {
      run_kjob(command);
      return 1;
    }
    else if(strcmp(command[0],"jobs") == 0)
    {
      run_jobs(command);
      return 1;
    }
    else if(strcmp(command[0],"fg") == 0)
    {
      run_fg(command);
      return 1;
    }
    else if(strcmp(command[0],"bg") == 0)
    {
      run_bg(command);
      return 1;
    }
    else if(strcmp(command[0],"overkill") == 0)
    {

      run_overkill(command);
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
  char** temp_parsed_string = parse_input_string(temp_input);
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

int run_pipe()
{
  int iter = 0,i, token_iterer = 0, pipestat;
  if(strcmp(parsed_line[0],"|") == 0)
  { printf("Syntax error.. | cant be in the beginning \n");
    return -1; }
  while(parsed_line[token_iterer] != NULL)
  {
    token_iterer += 1;
  }
  if(strcmp(parsed_line[token_iterer -1],"|") == 0)
  {
      printf("Syntax error.. | cant be in the end \n");
      return -1;
  }
  list_of_commands = malloc(sizeof(char**)*5); // Piping for max 5 commands

  int loc_iter = 0;
  list_of_commands[loc_iter] =  &parsed_line[0];
  while(parsed_line[iter] != NULL)
  {
    if(strcmp(parsed_line[iter],"|") == 0)
    {
      if(parsed_line[iter+1] != NULL){
        loc_iter ++; //next set of commands
      }
      list_of_commands[loc_iter] = &parsed_line[iter + 1];
      parsed_line[iter] = NULL;
    }
    iter++;
  }
  if(loc_iter == 0){return 0;}
  int pipe_arr[2*loc_iter],pipe_pid;
  for(iter = 0; iter < 2*loc_iter ;iter+=2)
  {
    if(pipe(pipe_arr + iter) != 0){perror("Pipe error\n");}
  }
  for(iter = 0; iter <= loc_iter ;iter++)
  {
    pipe_pid = fork();
    if(pipe_pid == -1){printf("Forking error \n"); exit(0);}
    else if(pipe_pid == 0)
    {
      if(iter != 0)
      {
        dup2(pipe_arr[2*iter - 2],0);
      }// For first proc no stdin
      if(iter != 2*loc_iter)
      {
        dup2(pipe_arr[2*(iter) + 1],1);
      }
      for(i = 0;i < 2*loc_iter ; i++)
      {
        close(pipe_arr[i]);
      }
      if(execvp(list_of_commands[iter][0],list_of_commands[iter]) < 0){perror("execvp error");}
    }
  }
  for(iter = 0; iter< 2*loc_iter; iter++){close(pipe_arr[iter]);}
  for(iter = 0; iter <= loc_iter; iter++){wait(&pipestat);}
  free(list_of_commands);
  return loc_iter;
}

int redirectify()
{
  int iter = 0; int small_iter; int outval = 1;
  while(parsed_line[iter] != NULL)
  {
    if(strcmp(parsed_line[iter],"<") == 0)
    {
      small_iter = iter ;
      inp_redirect_arr = malloc(sizeof(char)*100);
      strcpy(inp_redirect_arr,parsed_line[iter+1]);
      while(parsed_line[small_iter+2] != NULL)
      {
          parsed_line[small_iter] = parsed_line[small_iter + 2];
          small_iter++;
      }
      parsed_line[small_iter] = NULL;
    }
    iter++;
  }

  iter = 0;
  while(parsed_line[iter] != NULL)
  {
    if(strcmp(parsed_line[iter],">") == 0)
    {
      small_iter = iter ;
      out_redirect_arr = malloc(sizeof(char)*100);
      strcpy(out_redirect_arr,parsed_line[iter+1]);
      while(parsed_line[small_iter+2] != NULL)
      {
          parsed_line[small_iter] = parsed_line[small_iter + 2];
          small_iter++;
      }
      parsed_line[small_iter] = NULL;
    }
    iter++;
    }

  iter = 0;
  while(parsed_line[iter] != NULL)
  {
    if(strcmp(parsed_line[iter],">>") == 0)
    {
      small_iter = iter ;
      append_redirect_arr = malloc(sizeof(char)*100);
      strcpy(append_redirect_arr,parsed_line[iter+1]);
      while(parsed_line[small_iter+2] != NULL)
      {
          parsed_line[small_iter] = parsed_line[small_iter + 2];
          small_iter++;
      }
      parsed_line[small_iter] = NULL;
    }
    iter++;
  }
  if(inp_redirect_arr != NULL){
    close(0);
    if(open(inp_redirect_arr, O_RDONLY) == -1)
    {
      dup2(glob_stdin_copy,0);
      printf(ANSI_COLOR_RED "Invalid file for input redirection...\n" ANSI_COLOR_RESET);
			outval = -1;
		}
  }
  if(out_redirect_arr != NULL){
    close(1);
    outval = open(out_redirect_arr, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  }
  else if(append_redirect_arr != NULL){
    close(1);
    outval = open(append_redirect_arr, O_WRONLY | O_CREAT | O_APPEND, 0644);}
  return outval;
}

int close_redirectify()
{
  inp_redirect_arr = NULL;
  out_redirect_arr = NULL;
  append_redirect_arr = NULL;
  close(0);
  dup2(glob_stdin_copy,0);
  close(1);
  dup2(glob_stdout_copy,1);
  return 1;
}
