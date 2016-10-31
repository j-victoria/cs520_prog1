//definition for instruction class
#include "string.h"
#include "instructions.h"

using namespace std;
#include <string>

class Instruction {
  private:
    string instruction_name;  //i.e. ADD R1, R2, #3, the full instruction as a string
    instructions_t inst;  //ADD, BNZ, etc. see instructions.h for full list
    int dest, src1, src2, src3, lit, result;
    bool src1_valid, src2_valid, src3_valid;
  public:
    Instruction (string);
    Instruction() {};
    void read_in_str (string);
    void set_inst(string);
    string get_name();
};