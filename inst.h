//definition for instruction class
#include "string.h"
#include "instructions.h"

using namespace std;
#include <string>

class Instruction {
  private:
    string instruction_name;  //i.e. ADD R1, R2, #3, the full instruction as a string
    instructions_t inst;  //ADD, BNZ, etc. see instructions.h for full list
    int dest, src1, src2, src3, lit, res;
    bool src1_valid, src2_valid, src3_valid;
    int depends_on1, depends_on2, depends_on3; //refers to src1, 2 and 3 respectively
    int src1_ar, src2_ar, src3_ar;
  public:
    Instruction (string);
    Instruction() {};
    void read_in_str (string);
    int set_inst(string);
    string get_name();
    instructions_t get_int(); 
    int get_res();
    int get_dest();
    int get_srcs(int);
    void mark_as_valid(int);
    void mark_as_invalid(int);
    bool is_dependant();
    void clear ();
    void set_src(int);
    int get_src_ar(int i);
    void set_src(int i, int v);
    void set_src_ar(int i, int v);
    bool ready();
    int depends(int);
    int get_lit();
    void set_dest(int);
    void set_res(int);
    void set_lit(int);
    void create_dependency(int i, int v);
    bool is_valid(int);
    string printable_inst();
};