#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>

int status;
pid_t copy;
int red_fl=0;
int pipe_fl=0;
char home[1024];

typedef struct s
{
	struct s* next;
	char name[100];
	int pid;
}list;

list* head=NULL;

void insert_in_list(char name[],int pid)
{
	list* p;
	if (head==NULL)
	{
		p=malloc(sizeof(list));
		p->next=NULL;
		p->pid=pid;
		strcpy(p->name,name);
		head=p;
	}
	else
	{
		p=head;
		while(p->next!=NULL)
		{
			p=p->next;
		}
		list* q;
		q=malloc(sizeof(list));
		q->pid=pid;
		q->next=NULL;
		strcpy(q->name,name);
		p->next=q;
	}
}

void remove_from_list(int x)
{
	if (head==NULL)
	{
		printf("No Running Process!\n");
	}
	else
	{
		int fl=0;
		list* p=head;
		if (p->pid==x)
		{
			fl=1;
			head=NULL;
		}
		while(p->next!=NULL)
		{
			if(p->next->pid==x)
			{
				p->next=p->next->next;
				fl=1;
				break;
			}
		}
		if (fl==0)
		{
			printf("Invalid pid!\n");
		}
	}
}

typedef struct node
{
	char name[100];
	int type;
	int pid;
}node;

node all[10000];
int all_ct=0;
node back[10000];
int back_ct=0;

void prompt(char hostname[],char cwd[])
{
	char *name = getlogin();
	gethostname(hostname,1023);
	getcwd(cwd,1023);
	if (!strcmp(cwd,home))
	{
		printf("<%s@%s:~>",name,hostname);
	}
	else if (strstr(cwd,home))
	{
		char shortcwd[1000];
		int i,j=0;
		for(i=0;cwd[i]==home[i];i++)
		{
			//do nothing.
		}
		for(j=0;cwd[j];j++)
		{
			shortcwd[j]=cwd[i++];
		}
		printf("<%s@%s:~%s>",name,hostname,shortcwd);
	}
	else
	{
		printf("<%s@%s:%s>",name,hostname,cwd);
	}
}


void execute(char* input[],int flag)
{
	pid_t pid=fork();
	//	copy=pid;
	if (flag==0)
	{
		if (pid<0)
		{
			printf("Fork Failed!\n");
			exit(1);
		}
		else if (pid==0)
		{
			if (execvp(*input,input)<0)
			{
				printf("Command not found!\n");
				exit(0);
			}
		}
		else
		{
			all[all_ct].pid=pid;
			strcpy(all[all_ct].name,input[0]);
			all[all_ct].type=0;
			all_ct++;
			waitpid(pid,&status,WUNTRACED);
			if (WIFSTOPPED(status))
			{
				insert_in_list(input[0],pid);
				strcpy(back[back_ct].name,input[0]);
				back[back_ct].type=1;
				back[back_ct].pid=pid;
				back_ct++;
			}
		}
	}
	else if (flag==1)
	{
		if (pid<0)
		{
			printf("Fork Failed!\n");
			exit(1);
		}
		else if (pid==0)
		{
			if (execvp(*input,input)<0)
			{
				printf("Command failure\n");
				exit(0);
			}
		}
		else
		{
			strcpy(back[back_ct].name,input[0]);
			back[back_ct].pid=pid;
			back[back_ct].type=1;
			back_ct++;
			all[all_ct].pid=pid;
			strcpy(all[all_ct].name,input[0]);
			all[all_ct].type=1;
			all_ct++;
			insert_in_list(input[0],pid);
		}
	}
}

void stopper(int pid)
{
	if (!strcmp(all[all_ct-1].name,"fg"))
	{
		//printf("check\n");
		copy=back[back_ct-1].pid;
	}
	else
	{
		copy=all[all_ct-1].pid;
	}
	if (pid==SIGTSTP)
	{
		kill(copy,SIGTSTP);
	}
}

void only_red(char* input[])
{
	int stdin_org,stdout_org;//backup of file i/o handlers.
	char *re_out[1000],*re_in[1000],*arg[1000];
	int doub_fl=0,re_out_ct=0,re_in_ct=0,arg_ct=0;
	stdin_org=dup(STDIN_FILENO);
	stdout_org=dup(STDOUT_FILENO);
	
	int i;
	for (i=0;input[i];i++)
	{
		doub_fl=0;
		if (!strcmp(input[i],">") || !strcmp(input[i],">>"))
		{
			if (!strcmp(input[i],">>"))
			{
				doub_fl=1;
			}
			re_out[re_out_ct]=input[i+1];
			re_out[re_out_ct][strlen(input[i+1])]='\0';
			re_out_ct++;
			break;
		}
	}
	re_out[re_out_ct]=NULL;
	
	for (i=0;input[i];i++)
	{
		if (!strcmp(input[i],"<"))
		{
			re_in[re_in_ct]=input[i+1];
			re_in[re_in_ct][strlen(input[i+1])]='\0';
			re_in_ct++;
			break;
		}
	}
	re_in[re_in_ct]=NULL;

	for (i=0;input[i];i++)
	{
		if ( strcmp(input[i],">") && strcmp(input[i],"<") && strcmp(input[i],">>"))
		{
			if (re_in_ct > 0 && re_out_ct==0)
			{
				if (strcmp(input[i],re_in[0]))
				{
					arg[arg_ct]=input[i];
					arg_ct++;
				}
			}
			else if (re_out_ct > 0 && re_in_ct==0)
			{
				if (strcmp(input[i],re_out[0]))
				{
					arg[arg_ct]=input[i];
					arg_ct++;
				}
			}
			else
			{
				if (strcmp(input[i],re_out[0]) && strcmp(input[i],re_in[0]) ) 
				{
					arg[arg_ct]=input[i];
					arg_ct++;
				}
			}
		}
	}
	arg[arg_ct]=NULL;


	if (re_out_ct > 0 && re_in_ct == 0) //only output redirec.
	{
		pid_t pid;
		int out;
		if (doub_fl==0)
		{
			out=open(re_out[0], O_CREAT | O_TRUNC | O_WRONLY , S_IRWXU);
		}
		else if (doub_fl==1)
		{
			out=open(re_out[0], O_CREAT | O_APPEND | O_WRONLY, S_IRWXU);
		}
		if (out<0)
		{
			printf("Output file creation failed!\n");
			//		exit(1);
		}
		pid=fork();
		if (pid < 0)
		{
			printf("Fork Failed!\n");
			//		exit(1);
		}
		else if (pid==0)
		{
			all[all_ct].pid=pid;
			strcpy(all[all_ct].name,arg[0]);
			all[all_ct++].type=0;
			dup2(out, STDOUT_FILENO);
			if (execvp(*arg,arg)<0)
			{
				printf("Command not found!\n");
				//			exit(1);
			}
		}
		else
		{
			wait(NULL);
		}
	}
	else if (re_in_ct > 0 && re_out_ct == 0) //only input redirec.
	{
		pid_t pid;
		int in=open(re_in[0], O_RDONLY , 0664);
		if (in<0)
		{
			printf("Invalid Input file!\n");
			//		exit(1);
		}
		pid=fork();
		if (pid < 0)
		{
			printf("Fork Failed!\n");
			//		exit(1);
		}
		else if (pid == 0)
		{
			all[all_ct].pid=pid;
			strcpy(all[all_ct].name,arg[0]);
			all[all_ct++].type=0;
			dup2(in, STDIN_FILENO);
			if (execvp(*arg,arg)<0)
			{
				printf("Command not found!\n");
				//			exit(1);
			}
		}
		else 
		{
			wait(NULL);
		}
	}
	else //both input and output redirec.
	{
		//	printf("enters correct\n");
		pid_t pid;
		int in,out;
		in=open(re_in[0], O_RDONLY, 0664);
		if (doub_fl==0)
		{
			out=open(re_out[0], O_CREAT | O_TRUNC | O_WRONLY , S_IRWXU);
		}
		else if (doub_fl==1)
		{
			out=open(re_out[0], O_CREAT | O_APPEND | O_WRONLY, S_IRWXU);
		}
		if (in < 0)
		{
			printf("Invalid Input File!\n");
			//		exit(1);
		}
		if (out < 0)
		{
			printf("Outfile creation failed!\n");
			//		exit(1);
		}
		pid = fork();
		if (pid < 0)
		{
			printf("Fork Failed!\n");
			//		exit(1);
		}
		else if (pid == 0)
		{
			all[all_ct].pid=pid;
			strcpy(all[all_ct].name,arg[0]);
			all[all_ct++].type=0;
			//printf("in child\n");
			dup2(in,STDIN_FILENO);
			dup2(out,STDOUT_FILENO);
			int m;
		/*	for (m=0;arg[m];m++)
			{
				printf("%s\n",arg[m]);
			}*/
			if (execvp(*arg,arg)<0)
			{
				printf("Command not found!\n");
				//exit(1);
			}
		}
		else
		{
			wait(NULL);
		}
	}
	dup2(stdin_org,STDIN_FILENO);
	dup2(stdout_org,STDOUT_FILENO);
	close(stdin_org);
	close(stdout_org);
}


void pipe_and_red(char input[])
{
	char *token, temp[1000], *pipecomm[1000], *mohit[100], *args[1000], *newargs[1000];
	int p_ids[100],stdin_org,stdout_org,newpipe[2], mct=0, oldpipe[2], pipesct=0, argct, newargct, in=-1, out;
	pid_t pid;

	stdin_org=dup(STDIN_FILENO);
	stdout_org=dup(STDOUT_FILENO);

	token = strtok(input,"|");//parsing about |.
	while(token)
	{
		token[strlen(token)]='\0';
		pipecomm[pipesct++]=token;
		token=strtok(NULL,"|");
	}
	pipecomm[pipesct]=token;//pipecomm==cmd

	int i,in_fl,out_fl,in_ind,out_ind;
	for(i=0;i<pipesct;i++)
	{

		in_fl=out_fl=0;
		in_ind=out_ind=-1;
		int j;

		for (j=0;j<strlen(pipecomm[i]);j++)
		{
			if (pipecomm[i][j]=='>')
			{
				out_fl=1;
			}
			if (pipecomm[i][j]=='<')
			{
				in_fl=1;
			}
		}
		argct=0;

		if (in_fl)
		{
			token=strtok(pipecomm[i],"<");
			while(token)
			{
				token[strlen(token)]='\0';
				args[argct++]=token;
				token=strtok(NULL,"<");
			}
			args[argct]=token;
			
			args[1]=strtok(args[1]," ");//removing space from input file.
			/*while(token)
			{
				token[strlen(token)]='\0';
				mohit[mct++]=token;
				token=strtok(NULL," ");
			}
			mohit[mct]=token;*/

			newargct=0;

			token=strtok(args[0]," ");
			while(token)
			{
				token[strlen(token)]='\0';
				newargs[newargct++]=token;
				token=strtok(NULL," ");
			}
			newargs[newargct]=token;//newargs has the command with all its flags.

			in = open(args[1], O_RDONLY, 0664);
			if (in<0)
			{
				printf("Invalid Input File!\n");
				continue;
			}
			dup2(in,STDIN_FILENO);
		}
		else if (out_fl)
		{
			token=strtok(pipecomm[i],">");
			while(token)
			{
				token[strlen(token)]='\0';
				args[argct++]=token;
				token=strtok(NULL,">");
			}
			args[argct]=token;
			///////
			args[1]=strtok(args[1]," ");
			out=open(args[1], O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU);
			dup2(out,STDOUT_FILENO);
			args[1]=NULL;
			/*while(token)
			{
				token[strlen(token)]='\0';
				mohit[mct++]=token;
				token=strtok(NULL," ");
			}
			mohit[mct]=token;*/
			//printf("%s\n",mohit[0]);
			///////	
			newargct=0;
			token=strtok(args[0]," ");
			while(token)
			{
				token[strlen(token)]='\0';
				newargs[newargct++]=token;
				token=strtok(NULL," ");
			}
			newargs[newargct]=token;//newargs has the command with its flags.

			//out=open(args[1], O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU);
			if (out<0)
			{
				printf("Output file creation failed!\n");
				continue;
			}
			//dup2(out,STDOUT_FILENO);
		}

		else //only pipes.
		{
			token=strtok(pipecomm[i]," ");
			while(token)
			{
				token[strlen(token)]='\0';
				args[argct++]=token;
				token=strtok(NULL," ");
			}
			args[argct]=token;
		}

		if 	(i < pipesct-1)
		{
			pipe(newpipe);
		}

		pid=fork();
		if (i>0 && i<pipesct-1)
		{
			dup2(oldpipe[0],STDIN_FILENO);
			close(oldpipe[1]);
			close(oldpipe[0]);
		}
		else if (i==pipesct-1)
		{
			dup2(oldpipe[0],STDIN_FILENO);
			close(oldpipe[1]);
			close(oldpipe[0]);
		}


		if 	(pid<0)
		{
			printf("Child process creation failed!\n");
			continue;
		}
		else if (pid==0)
		{
			if (i==0)
			{
				dup2(newpipe[1],STDOUT_FILENO);
				close(newpipe[0]);
				close(newpipe[1]);
			}

			if (i>0 && i<pipesct-1)
			{
				dup2(newpipe[1],STDOUT_FILENO);
				close(newpipe[0]);
				close(newpipe[1]);
			}
		//	printf("args[0]%s\n",args[0]);
			/*char *jig[100];
			int jig_ct=0;
			printf("asd%sasd\n",args[0]);
			token=strtok(args[0]," ");
			while(token)
			{
				token[strlen(token)]='\0';
				jig[jig_ct]=token;
				token=strtok(NULL," ");
			}
			jig[jig_ct]=token;
			printf("asd%sasd\n",jig[0]);*/

			if (in_fl==0 && out_fl==0)
			{
				if (execvp(args[0],args)<0)//////////////////////////////////
				{
					printf("Command not found! %s\n",args[0]);
				}
			}
			if (out_fl)
			{
				if (execvp(newargs[0],newargs)<0)
				{
					printf("Command not found! %s\n",args[0]);
				}
			}
			if (in_fl)
			{
				if (execvp(newargs[0],newargs)<0)
				{
					printf("Command not found! %s\n",args[0]);
				}
			}
		}
		else if (pid>0)
		{
			waitpid(pid,&status,0);

		//	all[all_ct].pid=pid;
		//	strcpy(all[all_ct].name,args[0]);
		//	all[all_ct].type=0;

			if (i<pipesct-1)
			{
				oldpipe[0]=newpipe[0];
				oldpipe[1]=newpipe[1];
			}
		}
	}
	close(oldpipe[0]);
	close(newpipe[0]);
	close(oldpipe[1]);
	close(newpipe[1]);
	dup2(stdin_org,STDIN_FILENO);
	dup2(stdout_org,STDOUT_FILENO);
	close(stdin_org);
	close(stdout_org);
}


int main()
{
	printf("#####################################################################################################\n\n                    Welcome to Mohit's Terminal!\n\n#####################################################################################################\n\n");
	int i,chk;
	char ch;//ch is a waste character. and it should just go and die!
	pid_t pid;
	pid_t temp;
	temp=getpid();
	char comm[100],name,cwd[1024],hostname[1024];
	getcwd(home,1023);	//the original home!
	prompt(hostname,cwd);
	scanf("%[^\n]",comm);
	scanf("%c",&ch);
	signal(SIGINT,SIG_IGN);
	signal(SIGCHLD,SIG_IGN);
	signal(SIGQUIT,SIG_IGN);
	char commtemp[1000];
	strcpy(commtemp,comm);
	char *input[100];
	char *split;
	split=strtok(comm," ");
	for(i=0;split!=NULL;i++)//breaking the input as command line arguments agrc agrv type.
	{
		input[i]=split;
		split=strtok(NULL," ");
	}

	input[i]=NULL;
	while(1)
	{
		red_fl=0;
		pipe_fl=0;
		signal(SIGTSTP,stopper);
		for (i=0;input[i];i++)
		{
			if ((!strcmp(input[i],"<")) || (!strcmp(input[i],">")) || (!strcmp(input[i],">>")))
			{
				red_fl=1;
				break;
			}
		}
		for (i=0;input[i];i++)
		{
			if (!strcmp(input[i],"|"))
			{
				pipe_fl=1;
				break;
			}
		}
		if (!strcmp(input[0],"quit"))
		{
			break;
		}
		else if (pipe_fl==1)
		{
			pipe_fl=0;
			commtemp[strlen(commtemp)]='\0';
			pipe_and_red(commtemp);
		}
		else if (red_fl==1 && pipe_fl!=1)//only redirections.
		{
			red_fl=0;
			//printf("%s\n%s\n%s\n%s\n",input[0],input[1],input[2],input[3]);
			only_red(input);
		}
		else if (!strcmp(input[0],"pinfo"))//check if 'pinfo' is the command.
		{
			int fl=0;
			if (input[1]==NULL)
			{
				char cadd[100],addr[100],arr[100];
				sprintf(arr,"%d",temp);
				write(1,arr,1);
				strcpy(addr,"/proc/");
				strcat(addr,arr);
				strcpy(cadd,addr);
				strcat(addr,"/status");
				char* rt=NULL;
				size_t tp=0;
				FILE* fp;
				fp=fopen(addr,"r");
				getline(&rt,&tp,fp);
				printf("%s",rt);
				getline(&rt,&tp,fp);
				printf("%s",rt);
				getline(&rt,&tp,fp);
				getline(&rt,&tp,fp);
				printf("%s",rt);
				getline(&rt,&tp,fp);
				while(strncmp(rt,"VmSize",6))
				{
					if (!strncmp(rt,"Mems_allowed:",12))
					{
						fl=1;
						printf("Memory : ---\n");
						break;
					}
					getline(&rt,&tp,fp);
				}
				if (fl==0)
				{
					printf("%s",rt);
				}
				int sad;
				char new[1000];
				strcat(cadd,"/exe");
				sad=readlink(cadd,new,1000);
				new[sad]='\0';
				printf("Executable Path : %s\n",new);
			}
			else
			{
				char cadd[100],addr[100];
				strcpy(addr,"/proc/");
				strcat(addr,input[1]);
				strcpy(cadd,addr);
				strcat(addr,"/status");
				char* rt=NULL;
				size_t tp=0;
				FILE* fp;
				if (fopen(addr,"r"))
				{
					fp=fopen(addr,"r");
					getline(&rt,&tp,fp);
					printf("%s",rt);
					getline(&rt,&tp,fp);
					printf("%s",rt);
					getline(&rt,&tp,fp);
					getline(&rt,&tp,fp);
					printf("%s",rt);
					getline(&rt,&tp,fp);
					while(strncmp(rt,"VmSize",6))
					{
						if (!strncmp(rt,"Mems_allowed:",12))
						{
							fl=1;
							printf("Memory : ---\n");
							break;
						}
						getline(&rt,&tp,fp);
					}
					if (fl==0)
					{
						printf("%s",rt);
					}
					int sad;
					char new[1000];
					strcat(cadd,"/exe");
					sad=readlink(cadd,new,1000);
					new[sad]='\0';
					printf("Executable Path : %s\n",new);
				}
				else
				{
					printf("Invalid Pid!\n");
				}
			}
		}
		else if (!strcmp(input[0],"jobs"))
		{
			list* p=head;
			if (p!=NULL)
			{
				printf("List of currently running jobs :\n");
				while(p!=NULL)
				{
					printf("Process : %s Pid : %d\n",p->name,p->pid);
					p=p->next;
				}
			}
		}
		else if (!strcmp(input[0],"overkill"))
		{
			list* p=head;
			while(p!=NULL)
			{
				kill(p->pid,SIGKILL);
				p=p->next;
			}
			head=NULL;
		}
		else if (!strcmp(input[0],"fg"))
		{
			strcpy(all[all_ct].name,input[0]);
			all[all_ct].type=1;
			all[all_ct].pid=getpid();
			all_ct++;
			int num,fl=0;
			if (!input[1])
			{
				printf("PLease provide pid!\n");
			}
			else
			{
				num=atoi(input[1]);
				list* p=head;
				for(i=1;i<num;i++)
				{
					if (p && p->next!=NULL)
					{
						p=p->next;
					}
					else
					{
						fl=1;
						break;
					}
				}
				if (fl==0)
				{
					remove_from_list(p->pid);
					kill(p->pid,SIGCONT);
					wait(p->pid,SIGCONT);
				}
				else
				{
					printf("Invalid process number!\n");
				}
			}
		}
		else if (!strcmp(input[0],"history"))
		{
			strcpy(all[all_ct].name,input[0]);
			all[all_ct].type=1;
			all[all_ct].pid=getpid();
			all_ct++;
			if (all_ct)
			{
				printf("List of all processes called from this terminal :\n");
				for(i=0;i<all_ct;i++)
				{
					printf("[%d] Name : %s Process id : %d\n",i+1,all[i].name,all[i].pid);
				}
			}
			//	prompt(hostname,cwd);
		}
		else if (!strcmp(input[0],"backhistory"))
		{
			strcpy(all[all_ct].name,input[0]);
			all[all_ct].type=1;
			all[all_ct].pid=getpid();
			all_ct++;
			if (back_ct)
			{
				printf("List of all back-processes called from this terminal :\n");
				for (i=0;i<back_ct;i++)
				{
					printf("[%d] Name : %s Process id : %d\n",i+1,back[i].name,back[i].pid);
				}
			}
			//	prompt(hostname,cwd);
		}
		else if (!strcmp(input[0],"cd"))
		{
			strcpy(all[all_ct].name,input[0]);
			all[all_ct].type=1;
			all[all_ct].pid=getpid();
			all_ct++;
			if ((input[1]==NULL) || (!strcmp(input[1],"~")) || (!strcmp(input[1],"~/")))
			{
				chdir(home);
			}
			else
			{
				chdir(input[1]);
			}
			//	prompt(hostname,cwd);
		}
		else if (!strcmp(input[0],"kjob"))
		{
			int num,sig,fl=0;
			num=atoi(input[1]);
			sig=atoi(input[2]);
			list* p=head;
			for (i=1;i<num;i++)
			{
				if (p && p->next!=NULL)
				{
					p=p->next;
				}
				else
				{
					fl=1;
					break;
				}
			}
			if (fl==0)
			{
				remove_from_list(p->pid);	
				kill(p->pid,sig);
				wait(p->pid,sig);
			}
			else
			{
				printf("Invalid Job Number!\n");
			}
		}
		else 
		{
			int flag=0;
			if (input[1] && !(strcmp(input[1],"&")))
			{
				input[1]=NULL;
				flag=1;
			}
			execute(input,flag);
		}

		prompt(hostname,cwd);
		scanf("%[^\n]",comm);
		strcpy(commtemp,comm);
		scanf("%c",&ch);
		split=strtok(comm," ");//again breaking the input.
		for(i=0;split!=NULL;i++)
		{
			input[i]=split;
			split=strtok(NULL," ");
		}
		input[i]=NULL;
		chk=waitpid(-1,&status,WNOHANG);
		if (chk>0)
		{
			remove_from_list(chk);
			printf("Process [%d] exited normally.\n",chk);
		}
	}
	printf("#####################################################################################################\n\n                    Thanks for using Mohit's Terminal!\n\n#####################################################################################################\n\n");
	return 0;
}
