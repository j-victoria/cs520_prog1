//implimentation for Instruction class 

#include "inst.h"

#define s_to_i (str)  

Instruction::Instruction (string s) {
  instruction_name = s;
}
void Instruction::read_in_str (string s) {
  instruction_name = s;
}

void Instruction::set_inst(string s) {
 
  if (s == "ADD") inst = ADD;
  else if (s == "SUB") inst = SUB;
  else if (s == "MOVC") inst = MOVC;
  else if (s == "MUL") inst = MUL;
  else if (s == "AND") inst = AND;
  else if (s == "OR") inst = OR;
  else if (s == "EX_OR" || s == "EX-OR") inst = EX_OR;
  else if (s == "LOAD") inst = LOAD;
  else if (s == "STORE") inst = STORE;
  else if (s == "BZ") inst = BZ;
  else if (s == "BNZ") inst = BNZ;
  else if (s == "JUMP") inst = JUMP;
  else if (s == "BAL") inst = BAL;
  else inst = HALT;

  return;
}
string Instruction::get_name(){
  return instruction_name;
}