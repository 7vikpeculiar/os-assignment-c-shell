#include<stdio.h>
#include<stdlib.h>
#include<string.h>

int job_pntr = 0;
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

int printAllJobs()
{
	int iter = 0;
	procInfo* start = glob_Jobarr;
	while(start != NULL)
	{
		printf( "[%d] -----  %d [%s] \n" ,iter,start->proc_id, start->instr);
		start = start->next;
		iter++;
	}
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

int main()
{
	insertProc(12,"Whoare you");
	insertProc(13,"Whoam  you");
	insertProc(11,"Sdmw");
	printAllJobs();
	printf("[] [] %d\n", getJob(2));
	printAllJobs();
	return 0;
}
