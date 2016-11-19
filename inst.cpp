//implimentation for Instruction class 

#include "inst.h"

#define s_to_i (str)  

Instruction::Instruction (string s) {
  instruction_name = s;
}
void Instruction::read_in_str (string s) {
  instruction_name = s;
}

int Instruction::set_inst(string s) {
 
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
  else if (s == "HALT") inst = HALT;
  else return -1;
  
  return 0;
}

string Instruction::printable_inst(){
  switch (inst){
    case ADD : return "ADD"; break;
    case SUB : return "SUB"; break;
    case MOVC : return "MOVC"; break;
    case MUL : return "MUL"; break;
    case AND : return "AND"; break;
    case OR : return "OR"; break;
    case EX_OR : return "EX_OR"; break;
    case LOAD : return "LOAD"; break;
    case STORE : return "STORE"; break;
    case BZ : return "BZ"; break;
    case BNZ : return "BNZ"; break;
    case JUMP : return "JUMP"; break;
    case BAL : return "BAL"; break;
    case HALT: return "HALT"; break;
  }
}

string Instruction::get_name(){
  return instruction_name;
}

int Instruction::get_res() {
  return res;
}

instructions_t Instruction::get_int(){
  return inst;
}

int Instruction::get_dest(){
  return dest;
}

int Instruction::get_srcs(int i){
  switch(i){
    case 1 : return src1; break;
    case 2 : return src2; break;
  }
}

int Instruction::get_src_ar(int i){
    switch(i){
    case 1 : return src1_ar; break;
    case 2 : return src2_ar; break;
  }
}

void Instruction::mark_as_valid(int i){
  switch(i){
    case 1 : src1_valid = true; break;
    case 2 : src2_valid = true; break;
  }
}

void Instruction::mark_as_invalid(int i){
  switch(i){
    case 1 : src1_valid = false; break;
    case 2 : src2_valid = false; break;
  }
}

void Instruction::set_res(int i){
  res = i;
}

bool Instruction::is_dependant(){
  if (depends_on1 >= 0 || depends_on2 >= 0) return true;
  else return false;
}

int Instruction::depends(int i){
  switch(i){
    case 1 : return depends_on1; break;
    case 2 : return depends_on2; break;
    case 3 : return depends_on3; break;
  }
}

void Instruction::set_dest(int s) {
  dest = s;  
}

void Instruction::clear () {
  dest = src1 = src2 = src3 = lit = res = -1;
  src1_valid = src2_valid = src3_valid = false ;
  depends_on1 = depends_on2 = depends_on3 = -1;
  src1_ar = src2_ar = src3_ar = -1;
}

void Instruction::set_src(int i, int v){
  switch(i){
    case 1 : src1 = v; break;
    case 2 : src2 = v; break;
    case 3 : src3 = v; break;
  }
}


void Instruction::set_src_ar(int i, int v){
  switch(i){
    case 1 : src1_ar = v; break;
    case 2 : src2_ar = v; break;
    case 3 : src3_ar = v; break;
  }
}

bool Instruction::ready(){
  if(src1_valid && src2_valid) {
    return true;
  }
  else return false;
}

int  Instruction::get_lit(){
  return lit;
}

void Instruction::set_lit(int i){
  lit = i;
}

void Instruction::create_dependency(int i, int v){
  switch (i){
    case 1 : depends_on1 = v; break;
    case 2 : depends_on2 = v; break;
      
  }
}

bool Instruction::is_valid(int i){
  switch(i){
    case 1 : return src1_valid; break;
    case 2 : return src2_valid; break;
  }
}