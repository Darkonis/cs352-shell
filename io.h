/**
   Header file for the io.cpp file
   Author:Darron Anderson
   Date 10/11/19
*/
#ifndef IO_H
#define IO_H
#include <string>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <cstring>

/*
  initalize the global variables
 */
void init_variables(int* endflag,char* dir,char* run,bool* isFil);
/*
  parse a single line of input
  @param in the input line
  @param p[2] this is a pipe that i am not using and didn't get around to removeing
  @param trigger did the line include the & arguement
  @param more: if the more pipe is being used
 */
void parse_input( std::string in,int p[2],bool trigger,bool more);

#endif
