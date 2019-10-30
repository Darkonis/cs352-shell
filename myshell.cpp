/**
   The main file primarily responsible for parsing multiline
   comments and detecting things
   Author: Darron Anderson
   Date: 10/11/19
 */
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <iostream>
#include <string>
#include <sys/time.h>
#include <fstream>
#include <cstddef>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <fcntl.h>

#include "io.h"

/*
Setup various variables

*/
int endf=0;
int *endflag=&endf;
char* pwd;
char* shell;
bool* isFile;
extern char **environ;
int p[2];

// Function headers
void startup_sequence(int argC,char *args[]);
void setup_IO(int argC, char* args[]);
bool check_last_char(std::string in);
bool check_output_redir(std::string in);
void handle_redirects(std::string in,int p[2],bool trigger);
bool check_more(std::string in);

/**
   I really hope this function is obvious and doesn't need an explaniation
 */
int main(int argc, char *argv[])
{
   isFile=(bool*)malloc(1);
  *isFile=false;
  //std::streambuf *cinbuf = std::cin.rdbuf();
  /*
    detect if a file is to be the input
   */
  std::ifstream in;
  if(argc==2)
    {
      in.open(argv[1]);
      std::cin.rdbuf(in.rdbuf());

      *isFile=true;

    }
  /*
    initalize global variables
   */
  startup_sequence(argc,argv);
   int coutbuf =dup(1);
  int errbuf =dup(2);
  int inbuf =dup(0);
  /*
    The main loop of the file which will run until quit is entered or if the input is a file then it will run until the file is ended
   */
  while((!*isFile&&*endflag!=1)||(*isFile && !std::cin.eof()))
    {
      dup2(inbuf,0);
      dup2(coutbuf,1);
      dup2(errbuf,2);
      waitpid(-1, NULL, WNOHANG);
      std::string input="";
      if(!*isFile) std::cout<<pwd<<":";//If the input is in user control mode then output the current directory
      std::getline(std::cin, input);//Get the next line of the input
      if(input.compare("")==0) continue;
      bool trigger=check_last_char(input);//Check to see if the last character of a line is & and if so trigger (hence the name) concurrent mode
      if(trigger)
	{
	  input = input.substr(0,input.find_last_of("&"));//remove the & if the trigger is detected
	}
      std::string in = input;
      while(input.find(';')!=std::string::npos )
	{
	  in=input.substr(0,input.find(';')); //Remove the first input from a string of inputs
          input = input.substr(input.find(';')+1,input.length());
	  handle_redirects(in,p,trigger);
	}
      in=input;
      handle_redirects(in,p,trigger);
    }
  //quick thank you
  printf("Thank you for using this shell.\n");
  //close the input file if it was opened
  if(argc==2)
    {
      in.close();
    }
  //Free all mallocs
  free(pwd);
  free(shell);
  free(isFile);
}
/**
   sets up enviorment variables 
   @param argC the number of args
   @param args[] the list of arguements
 */
void startup_sequence(int argC,char* args[])
{
  long path_max;
  size_t size;
  //Determine path sieze
  path_max = pathconf(".", _PC_PATH_MAX);
  if (path_max == -1)
    size = 1024;
  else if (path_max > 10240)
    size = 10240;
  else
    size = path_max;
  //Malloc needed space
  pwd=(char*)malloc(size);
  shell = (char*) malloc(size+sizeof("/myShell"));
  getcwd(pwd,size);
  getcwd(shell,size);
  //create the path this file was run from
  strcat(shell,"/myShell");
  setenv("shell",shell , 1);
  init_variables(endflag,pwd,shell,isFile);//init variables in the io file
}
/**
   Isolates a section of a word given a start point
   Will not work if start is not in the word
   @param string in: the string to isolate from
   @param char* start: the char seqence to start with
 */
std::string isolate(std::string in,const char* start)
{
  std::string ret=in.substr(in.find(start)+strlen(start),in.length());
  while(ret.find(' ')==0)
    {
      ret=ret.substr(1);
    }
  if (ret.find(' ')!=std::string::npos) ret = ret.substr(0,ret.find(' '));
  return ret;
}
/*
  parse the input and see if the last non space character is a &
  @param in: the input string
 */
bool check_last_char(std::string in)
{
  while(in.at(in.length()-1)==' ')
    {
      in=in.substr(0,in.length()-1);
    }
  return in.at(in.length()-1)=='&';
  
}
/*
  check if > is in the line
  and if it is redirect output and error
  @param in: the input string
  @param outFile: the reference to the outFile if needed
  @return returns if output redirection happened
 */
bool handle_output_redir(std::string in,int* outFile)
{
  bool out = in.find(" >")!=std::string::npos;
  if(out)
    {
      //change output if needed
      bool tmp = in.find(" >>")!=std::string::npos; //if the " >>" is present not just " >"
      if (tmp)
	{
	  std::string search=" >>";
	  std::string outfil_n=isolate(in,search.c_str());// parse the output file       
	  *outFile= open(outfil_n.c_str(),O_RDWR|O_CREAT,0644); //open the output file with proper flags    
	  dup2(*outFile,1);//redirect output to the file      
	  dup2(*outFile,2);
	}
      else
	{
	  //Essentailly the same as above but with different flags
	  std::string search=" >";
	  std::string outfil_n=isolate(in,search.c_str());
	  *outFile=open(outfil_n.c_str(),O_RDWR|O_CREAT|O_TRUNC,0644);
	  dup2(*outFile,1);
	  dup2(*outFile,2);
	}
      return true;
    }
  return false;
}
/**
 check if < is in the line                                                                                                                                                            
  and if it is redirect input  
  @param in: the input string                                                                                                                                                       
  @param inFile: the reference to the inFile if needed  
  @return return if input redirection happend
*/
bool handle_input_redir(std::string in,int* inFile)
{
  bool inRe = in.find(" <")!=std::string::npos;
  if(inRe)
    {
      std::string search=" <";
      std::string outfil_n=isolate(in,search.c_str());
      *inFile=open(outfil_n.c_str(),std::fstream::in);
      dup2(*inFile,0);
      return true;
    }
  return false;
}
/**
   Handle and needed IO redirection then parse the input
   @param in: the input string
   @param p[2]: a pipe if needed
   @param trigger: if a & was detected
 */
void handle_redirects(std::string in,int p[2],bool trigger)
{
  int coutbuf =dup(1);
  int errbuf =dup(2);
  int inbuf =dup(0);
  if (coutbuf ==-1||errbuf==-1||inbuf==-1)
    {
      std::cout<<"Dup error\n";
      return;
    }
  int* outFile=(int*)malloc(sizeof(int*));
  int* inFile = (int*)malloc(sizeof(int*));
  bool out = handle_output_redir(in,outFile);
  bool in_flag = handle_input_redir(in,inFile);
  //Trim the line of any > or <
  if(out)
    {
      in = in.substr(0,in.find(" >"));
    }
  if(in.find(" <")!=std::string::npos)
    {
      in = in.substr(0,in.find(" <"));
    }
  bool more = check_more(in);
  if(more&&in.find("|more")!=std::string::npos)//handle more if the line was |more
    {
      std::string p1;
      std::string p2;
      p1 = in.substr(0,in.find("|more"));
      p2 = in.substr(in.find("|more")+5);
      in = p1+p2;
    }
  else if(more && in.find("| more")!=std::string::npos)//handle more if line was | more
    {
      std::string p1;
      std::string p2;
      p1 = in.substr(0,in.find("| more"));
      p2 = in.substr(in.find("| more")+6);
      in = p1+p2;

    }
  parse_input(in,p,trigger,more);//DO THE THING on a serious note this could be better named as process and execute input
  //fix any input redirection
  dup2(inbuf,0);
  dup2(coutbuf,1);
  dup2(errbuf,2);
  //close any opened files
  if(out)
    {
      close(*outFile);
    }
  if(in_flag)
    {
      close(*inFile);
    }
  free(outFile);
  free(inFile);
}
/**
   check if the line needs piping to more
   @param in: the input string
   @return a boolean thats true if piping is needed
 */
bool check_more(std::string in)
{
  if(in.find("help")!=std::string::npos) return true;
  if(in.find("|more")==std::string::npos&&in.find("| more")==std::string::npos)
    {
      return false;
    }
  return true;
}
