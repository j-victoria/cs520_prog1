//structs
//#include "inst.h"
#include "instructions.h"
using namespace std;
#include <string>

typedef struct instruction_t {
  int dest, src1_v, src1_a, src2_v, src2_a, lit, res, index, dc, rob_loc, dest_ar, iq_loc, latch_loc;
  bool src1, src2, valid, in_iq, branch_taken;
  enum instructions_t type;
  string name;
}Instruction;

typedef struct reg {
  vector<Instruction *> consumer1;
  vector<Instruction *> consumer2;
  int value;
  bool valid;
  Instruction * producer;
}Register;

typedef struct iq_entry {
  Instruction * inst;
  bool valid;
}iqe;

typedef struct rat_entry {
  Register * reg;
  Instruction * producer;
  vector <Instruction> consumers;
}rate;

typedef struct rob_entry {
  Instruction * inst;
  bool finished;
  bool valid;
  int renaming;
}robe;

