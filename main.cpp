//main program
//used for testing for now

using namespace std;

#include <vector>
#include <fstream>
#include <iostream>
#include <string>

//local files
#include "inst.h"
#include "registers.h"

//globals
Register * rf[16];
vector<Instruction> pc_int;
int memory[1000];

//functions
int access_memory_at (int i){
  return memory[i/4];
}
int get_inst_from_file (const char *);
int fetch (int);

//main
int main () {
  
  get_inst_from_file("test_inst");
  
  fetch(1);
  
  return 0;
}


int get_inst_from_file (const char * file_name){
  ifstream inst_file;
  string inst;
  
  inst_file.open(file_name);
  if (!(inst_file.is_open())){
    return -1;
  }
  while(getline(inst_file, inst)){
    pc_int.push_back(Instruction(inst));
  }  
  
  inst_file.close();
  
  return 0;
}

//in progress!! -J
int fetch (int inst_index) {
  Instruction i = pc_int[inst_index];
  string s = i.get_name();
  i.set_inst(s.substr(0, s.find(" ")));
  string rest = s.substr(s.find(" "), s.length());
  if (rest[0] == 'R'){  //register
    
  }
  else if (rest[0] == '#'){ //literal
    
  }
  return 0;
}