//definition for instruction class
#include "string.h"
#include "instructions.h"

class Instruction {
  private:
    const char * instruction_name;  //i.e. ADD R1, R2, #3, the full instruction as a string
    instructions_t inst;  //ADD, BNZ, etc. see instructions.h for full list
    int dest, src1, src2, src3, lit;
  public:
    Instruction (const char *);
};