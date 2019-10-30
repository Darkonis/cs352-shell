/**
   This functions handles the parsing and running of a single command
   Author Darron Anderson
   Date 10/11/2019
 */
#include "io.h"
#include <iostream>
#include <unistd.h>
int handle_command(std::string command,std::string args,bool trigger,int p[2]);

//Main variables
int* end;
char* dirRef;
char* runRef;
bool* isF;
extern char **environ;
bool moreF;
int mpid =-1;
/**
Parse_input takes a single command inout for instance ls -l
parses it and passes it to the handle command function for execution

@param string in: the line of input
@param int p: a pipe if needed
@param bool trigger: was an '&' detected in this command line
@param more did this command have a |more in the line
*/
void parse_input( std::string in,int p[2],bool trigger,bool more)
{
  std::string command = in;
  std::string args;
  int save=dup(0);
  int save2=dup(1);
  if(save ==-1 ||save2 ==-1)
    {
      std::cout<<"backup error l23\n";
      return;
    }
  moreF=more;//set a flag
  if(more) //handle more piping
    {
      int perr=pipe(p);
      if(perr==-1)
	{
	  std::cout<<"Pipe error\n";
	  return;
	}
      mpid=fork();
      if(mpid <0)
	{
	  std::cout<<"error forking for more\n";
	  return;
	}
      if(mpid>0)
	{
	  close(p[0]);
	  if(dup2(p[1],1)==-1) std::cout<<"more redirect through dup error";
	  close(p[1]);
	}
      else
	{
	  close(p[1]);
	  if(dup2(p[0],0)==-1) std::cout<<"more redirect through dup error";
	  close(p[0]);
	  char * ls_args[] = {(char*)"more" , NULL};
          execvp(ls_args[0],ls_args);
	  std::cout<<"exec error \n";
	  exit(0);
	}
    }
  while(command.find(' ')==0)//trim spaces
    {
      command=command.substr(1,command.length());
    }
  if(command.find(' ')!=std::string::npos)//trim ending spaces
    {
      args=command.substr(command.find(" ")+1,command.length());
      command=command.substr(0,command.find(" "));
    }
  if(command.compare("quit")==0) //quit special case
    {
      *end=1;
      return;
    }
  else{
    handle_command(command,args,trigger,p); //do the thing
    if(more)//if more wait for it to close
      {
	close(p[0]);
	close(p[1]);
	dup2(save,0);
	dup2(save2,1);
	wait(NULL);
      }
    dup2(save,0);//reset the input
    dup2(save2,1);//reset the output
  }
  return;
}
/**
   Take a parsed command and execute it
   @param string command: the parsed command
   @param string args: the arguements for this function
   @param bool trigger: was an & in the command line
   @param int p[2]: the pipe
   @return an error indicator if something goes wrong
 */
int handle_command(std::string command,std::string args,bool trigger,int p[2])
{
  
  if(command.compare("help")==0)//help handling
      {
	char * ls_args[] = {(char*)"man" ,(char*)args.c_str(), NULL};//set up args
        if(std::strcmp(ls_args[1],"")==0) ls_args[1]=NULL;//null if needed
        int pid=fork();
	if(pid <0)
	  {
	    std::cout<<"error forking\n";
	    return -1;
	  }
	if(pid==0)
          {
            std::cout<<args<<"\n";
            execvp(ls_args[0],ls_args);
	    std::cout<<"exec error\n";
            exit(0);
          }
        else
          {
	    if(!trigger) wait(NULL);//only wait if there was no &
            return 0;
          }
      }
  if(command.compare("pause")==0) // pause handling
      {
	std::cout<<"System is now paused press enter to continue\n";
	while(getchar()!='\n')
	  {
	  }
	return 0;
      }
  if(command.compare("environ")==0) //environ handling
      {
	int i = 1;
	char *s = *environ;
	for (; s; i++) {
	  std::cout<<s<<"\n";
	  s = *(environ+i);
	}
	
	return 0;
      }
  if(command.compare("cd")==0)//cd handling
      {
	if(args.compare("")==0)
	  {
	    std::cout<<"Current directory is:"<<dirRef<<"\n";
	    return 0;
	  }
	int i=chdir(args.c_str());
	if(i<0) std::cout<<"bash: cd: "<<args.c_str()<<": No such file or directory\n"; 
	getcwd(dirRef,10240);
	return 0;
      }
  char * ls_args[] = {(char*)command.c_str() ,(char*)args.c_str(), NULL}; //all other commands handling
	if(std::strcmp(ls_args[1],"")==0) ls_args[1]=NULL;
	int pid=fork();
	if(pid<0)
	  {
	    std::cout<<"forking error\n";
	    return -1;
	  }
	if(pid==0)
	  {
	    if (command.compare("dir")==0) command="ls"; //handle dir
	    if (command.compare("clr")==0) command="clear";//hanlde clr
	    setenv("parent",runRef , 1);
	    if(execvp(command.c_str(),ls_args)==-1);
	    std::cout<<"exec error this may be due to an invalid command\n";
	    exit(0);
	  }
	else
	  {
	    if(!trigger) wait(NULL);
	    close(p[1]);//close pipe if needed
	    close(p[0]);//close pipe if needed
	    getcwd(dirRef,12040);//update the directory (probably unnessassary)
	    return 0;
	  }
	//default case should never be reached
  std::cout<<command<<" is currently an unsupported command\n"<<"Get Help\n";
  return 0;
}
/**
   initalize global variables
   @param int* endflag: the flag to end the program
   @param char* dir: the address holding the cwd
   @param char* run: the address holding the directory this was run from
   @param bool* isFil: the boolean for if the input is coming from a file
 */
void init_variables(int* endflag,char* dir,char* run,bool* isFil)
{
  dirRef=dir;
  end=endflag;
  runRef=run;
  isF=isFil;
}
