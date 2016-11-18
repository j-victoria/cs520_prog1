//main program
//used for testing for now


using namespace std;

#include <vector>
#include <fstream>
#include <iostream>
#include <string>
#include <cstdlib>
#include <algorithm>
#include <bitset>
#include <assert.h>

//local files
#include "inst.h"
#include "registers.h"

//constants/macros
#define FETCH 1
#define DRF 2
#define ALU1 3
#define ALU2 4
#define BEU 5   //branch execution unit
#define DELAY 6
#define MEM 7
#define WB 8

#define ZERO_FLAG 17
#define REG_X 18

#define EOP -2 // used to signal end of processing

#define ND -1 //used when a field is not in use 

//globals
Register rf[20];
vector<Instruction> pc_int;
int memory[1000];
int fwd_val [9][3]; //where [MEM][0] is the dest. of a fwd'd value resulting from the mem stage
bool stall [9]; // true if there is a stall in the ith stage
                //only a stage can write to its own stall
                // MV: Changed [8] to [9] so that stall[8] is valid for stall[WB] - stages can be renumbered if necessary? Is 0 in use elsewhere?
int curr_pc[9];
int next_pc[9]; //need to scrub at every new 

bool squash;  //when true: fetch and decode are not executed 
int z_ptr;    //points to the instrucion that last affected the Z 

bool dirty_latch[9]; // records whether the latch has been written to this cycle true if written false if not

bool debug;

//functions
int access_memory_at (int i){
  return memory[i/4];
}

int write_to_mem (int i, int v){
  memory[i/4] = v;
}

int is_zero (int v){
  return (v == 0);
}

void display_latch(int * latch){
  cout << "Fetch:" << latch[FETCH] << " Decode:" << latch[DRF] << " ALU 1:" << latch[ALU1] << " ALU2 :" << latch[ALU2] << endl;
  cout << "Brach FU:" << latch[BEU] << " Delay:" << latch[DELAY] << " Memory:" << latch[MEM] << " Write Back:" << latch[WB] << endl;
  return;
}

void display_rf(Register * rf_star){
  size_t l = sizeof(&rf_star)/sizeof(Register);
  for (int i = 0; i < l; i++){
    cout << "AR " << i << ": " << rf_star[i].read_value() << " " << ((rf_star[i].read_valid_bit())?"v":"i") << " " << ((rf_star[i].read_z())?"z":"n") << " ";
    if (!(i % 4)) cout << endl;
  }
  return;
}

void print_mem(int max){
  for (int i = 0; i < (max/4); i++){
    cout << i * 4 << ": " << memory[i] << " ";
    if(!(i % 8)) cout << endl;
  }
    
}

void clean_up(){
  for (int i = FETCH; i < WB + 1; i++){
    if(!(dirty_latch[i])){
      next_pc[i] = ND;
    }
    dirty_latch[i] = false;
  }
  return;
}

int get_inst_from_file (const char *);
int fetch (int);
int decode (int);
int alu1 (int);
int alu2 (int);
int beu (int);
int delay (int);
int mem (int);
int wb (int);

//main
int main (int argc, char *argv[]) {
  
  if (argc > 2){
    if (argv[2])
    debug = (argv[2] == "d");
  } else debug = false;
  
  string in;
  int rv;
  int n;
  //set up globals
  memset(stall, false, sizeof(stall));
  
  for(int i = 0; i < 9; i++){
    stall[i] = false;
  }
  
  for (int i = 0; i < 20; i++){
    rf[i].reset();
  }
  
  squash = false;
  z_ptr = ND;
  
  for(int i = 0; i < WB + 1; i++){
    fwd_val[i][0] = fwd_val[i][1] = fwd_val[i][2] = ND;
  }
  
  in = argv[1];
  cout << "Reading file ..." << cout;
  if (get_inst_from_file(in.c_str()) != 0) {cout << "error opening file" << endl; return 0;}
  cout << "There are " << pc_int.size() << " instructions in " << in  << endl;
  memset(next_pc, -1, sizeof(next_pc));
  next_pc[FETCH] = 0;
  int wbrv;
  do{
    cout << "Pick a mode Initilize, Simulate, or Display: ";
    getline(cin, in);
    if (in.compare("Simulate") == 0){
      cout << "Starting exectution..." << endl;
      int j = in.find_first_of("0123456789", 0);
      int cycles = 0;
      do{
        cycles = cycles * 10 +  ((int) in[j] - 48);
      }while(isdigit(in[++j]));
      do{
        //copy the new pc addresses into the current pc addr buffer
        //acts as latch
        //this is the only time the curr_pc is written to
        memcpy(curr_pc, next_pc, sizeof(curr_pc));

        display_latch(curr_pc);

        wbrv = wb(curr_pc[WB]); assert (wbrv == 0);
        rv = mem(curr_pc[MEM]); assert (rv == 0);
        rv = alu2(curr_pc[ALU2]); assert (rv == 0);
        rv = delay(curr_pc[DELAY]); assert (rv == 0);
        rv = alu1(curr_pc[ALU1]); assert (rv == 0);
        rv = beu(curr_pc[BEU]); assert (rv == 0);
        rv = decode(curr_pc[DRF]); assert (rv == 0);
        //cout << pc_int[curr_pc[FETCH]].get_src_ar(1) << endl;
        rv = fetch(curr_pc[FETCH]);
        //cout << pc_int[curr_pc[FETCH]].get_src_ar(1) << endl;

        clean_up();
        //cin.ignore();

      }while (n++ < cycles);
      n = 0;
    } else if (in.compare("Initilize")) {
      
    }
  }while (true);
  return 0;
}


/**
 * get_inst_from_file
 * Reads in lines from a text file and ###
 */
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

/**
 * fetch
 * Simulates the FETCH stage.
 */
int fetch (int inst_index) {
  if (squash) { inst_index = ND; cout << "Fetch is squashed " << endl;} 
  
  if (stall[FETCH]){
    stall[FETCH] = false;
    cout << "Stall in Fetch" << endl;
  } else {
    if (!(inst_index == ND)){
      Instruction i = pc_int[inst_index];
      string s = i.get_name();

      i.set_inst(s.substr(0, s.find(" ")));
      cout << inst_index << ": Fetch determined instruction to be a "<< i.printable_inst() << endl;
      string rest = s.substr(s.find(" "), s.length() - 1);

      //cout << rest << endl;

      if (i.get_int() == JUMP && rest.find("X", 0) != string::npos){
        // handle special case here

        i.set_dest(ND); 
        i.set_src_ar(1, REG_X);
        i.set_src_ar(2, ND);
        

      } else {



        int j = 0, r = 1;
        instructions_t inst = i.get_int();

        //set up default case 
        i.set_dest(ND); i.set_src_ar(1, ND); i.set_src_ar(2, ND); i.set_lit(ND); i.set_res(ND);
        //this should make decode slightly easier


        j = rest.find_first_of("0123456789", j + 1);
        while (j < rest.length()){
          int reg = 0;
          int sign = 1;
          if (rest[j - 1] == '-') sign = -1;

          while ( isdigit( rest[j] ) ) {
            reg = reg * 10 + ((int) rest[j] - 48);
            j++;
          }
          //cout << "val :" << reg << endl;
          reg = reg * sign;
          if(r == 1){
            if(inst == ADD || inst == SUB || inst == MUL || inst == AND || inst == OR || inst == EX_OR || inst == LOAD || inst == MOVC){
              //write to register inst.
              i.set_dest(reg);
              //will mark as invalid in decode
            } else if (inst == STORE || inst == BAL || inst == JUMP){
              i.set_src_ar(1, reg); i.set_dest(ND);
            } else if (inst == BZ || inst == BNZ){
              i.set_lit(reg); i.set_src(1, REG_X);  

            }else {
              //error case
            }


          } else if (r == 2) {
            if (inst == ADD || inst == SUB || inst == MUL || inst == AND || inst == OR || inst == EX_OR || inst == LOAD){
              //1st reg src
              i.set_src_ar(1, reg);
            } else if (inst == STORE) {
              i.set_src_ar(2, reg);
            }else if (inst == MOVC || inst == JUMP || inst == BAL){
              //

              i.set_lit(reg);
            }else {
              // error case
            }

          } else {
            //r == 3
            if (inst == LOAD || inst == STORE){
              i.set_lit(reg);
            } else if (inst == ADD || inst == SUB || inst == MUL || inst == AND || inst == OR || inst == EX_OR){
              i.set_src_ar(2, reg);
            } else {
              //error case
            }

          }
          r++;

          j = rest.find_first_of("0123456789", j + 1);
        }
      }
      if (i.get_dest() != ND){
        cout << inst_index <<": Fetch determined destination to be AR "<< i.get_dest() << endl;
      } else {
        cout << inst_index << ": Fetch did not discover a destination" << endl;
      }
     if (i.get_src_ar(1) != ND){
        cout << inst_index <<": Fetch determined source 1 to be AR "<< i.get_src_ar(1) << endl;
      } else {
        cout << inst_index << ": Fetch did not discover a source 1" << endl;
      }
      if (i.get_src_ar(2) != ND){
        cout << inst_index <<": Fetch determined source 2 to be AR "<< i.get_src_ar(2) << endl;
      } else {
        cout << inst_index << ": Fetch did not discover a source 2" << endl;
      }
      if (i.get_lit() != ND){
        cout << inst_index <<": Fetch determined literal to be "<< i.get_lit() << endl;
      } else {
        cout << inst_index << ": Fetch did not discover a literal" << endl;
      }
    pc_int[inst_index] = i;
    } else {cout << "Fetch did nothing..." << endl;} // end if nd
  }// end if stall
  
  
  //maybe combine these?
  if (squash){
    squash = false;
  } else if (stall[DRF]){
    //not squashed but drfis stalled
    stall[FETCH]= true;
    next_pc[FETCH] = inst_index;
    dirty_latch[FETCH] = true;
  } else if (inst_index + 1 >= pc_int.size()) { // need to prevent ourselves from going over the edge
    //drf is not stalled, but instruction is the last line in the file
    next_pc[FETCH] = ND;
    dirty_latch[FETCH] = true;
  } else if (inst_index == ND){
    //no stall, not squashed, i = ND
    next_pc[FETCH] = ND;
    dirty_latch[FETCH] = true;
  } else  {
    //No stall, not squashed, the next sequential instruction exists, and i != ND
    next_pc[FETCH] = inst_index + 1;
    dirty_latch[FETCH] = true;
    next_pc[DRF] = inst_index;
    dirty_latch[DRF] = true;
  }
  
  return 0;
}

/**
 * decode
 * Simulates the D/RF stage.
 */
int decode (int i){
  
  if(squash) {i = ND; cout << "Decode is squashed" << endl;}
  
  if (i != ND){
    
    
    Instruction d = pc_int[i];
    
    if (stall[DRF]){
      stall[DRF] = false;
      cout << "Decode is stalled..." << endl;

    }  else {
      cout << i << ": Decode ready to decode!" << endl;
      d.create_dependency(1, ND); d.create_dependency(2, ND);//assume no dependencies   
      d.mark_as_valid(1); d.mark_as_valid(2);
      int dep;
      for (int j = 1; j < 3; j++){
        if (d.get_src_ar(j) != ND){
          cout << i << ": Decode detected a source! AR " << d.get_src_ar(j) << endl;
          
          if (rf[d.get_src_ar(j)].read_valid_bit()){
            d.set_src(j, rf[d.get_src_ar(j)].read_value());
            d.mark_as_valid(j);
            cout << i <<": Decode read soruce " << j << " from register " << d.get_src_ar(j) << ": " << d.get_srcs(j) << endl;
          } else {
            cout << i << ": Decode could not find source " << j << " in the register file!" << endl;
            //we have to walk forward through the stages to find the instruction we are  dependant on
            if (pc_int[curr_pc[ALU1]].get_dest() == d.get_src_ar(j)){
              dep = curr_pc[ALU1];
              
            }else if (pc_int[curr_pc[BEU]].get_dest() == d.get_src_ar(j)) {
              dep = curr_pc[BEU];
            } else if (pc_int[curr_pc[ALU2]].get_dest() == d.get_src_ar(j)) {
              dep = curr_pc[ALU2];
            } else if (pc_int[curr_pc[DELAY]].get_dest() == d.get_src_ar(j)) {
              dep = curr_pc[DELAY];
            } else if (pc_int[curr_pc[MEM]].get_dest() == d.get_src_ar(j)) {
              dep = curr_pc[MEM];
            } else if (pc_int[curr_pc[WB]].get_dest() == d.get_src_ar(j)) {
              dep = curr_pc[WB];
            } else {
              //error case
            }
            cout << i << ": Decode detected a dependency between " << i << " and " << dep << endl;
            d.create_dependency(j, dep);
            d.mark_as_invalid(j);
          }
        } else {d.set_src(j, ND); d.mark_as_valid(j); cout << i << ": No source " << j << " to retreive" << endl;}
      }
      if (d.get_dest() != ND){
        //mark destination register as invalid
        cout << i << ": Writing valid bit..." << endl;
        rf[d.get_dest()].write_valid_bit(false);
        cout << i << ": Decode marked AR " << d.get_dest() << " as invaild" << endl;
      } else {cout << i << ": No destination to mark..." << endl;}
      
      // zero flag stuff!!!
      if (d.get_int() == ADD || d.get_int() == SUB || d.get_int() == MUL){
        z_ptr = i;
        cout << i << ": decode detected an arithmetic instruction! Arranged zero flag!" << endl;
      }
      if (d.get_int() == BNZ || d.get_int() == BZ){
        if(z_ptr == curr_pc[ALU1]){  
          d.create_dependency(1, z_ptr);
          d.mark_as_invalid(1);
          cout << i << ": Decode detected a dependency between " << i << " and " << curr_pc[ALU1] << endl;
        } else if (rf[pc_int[z_ptr].get_dest()].read_valid_bit()) {
          d.set_src(1, rf[pc_int[z_ptr].get_dest()].read_z());
          d.mark_as_valid(1);
          cout << i << ": Decode read the zero flage from AR " << z_ptr << endl;
        } else if(pc_int[curr_pc[ALU2]].get_dest() == z_ptr){
          d.set_src(1, fwd_val[ALU2][1]);
          d.mark_as_valid(1);
          cout << i << ": Decode read value forwarded from ALU 2!" << endl;
        } else if (pc_int[curr_pc[MEM]].get_dest() == z_ptr){
          d.set_src(1, fwd_val[MEM][1]);
          d.mark_as_valid(1);
          cout << i << ": Decode read value forwarded from Memory!" << endl;
        }
      }
        
        // X register stuff
        if (d.get_int() == BAL) {
          d.set_dest(REG_X);
          rf[REG_X].write_valid_bit(false);
          cout << i << ": Decode marked AR X as invalid!" << endl;
        } //jump's dependency is taken care of above, since X is just a regular register with a special name :/

      } // end else stall
     
    
    int p;
    
    //issusing 
   instructions_t di = d.get_int();
    if (di == BAL || di == JUMP || di == BZ || di == BNZ){
      cout << i << ": Evaluating branch execution readiness" << endl;
      if(stall[BEU]){
        stall[DRF] = true;
        next_pc[DRF] = i;
        cout << i << ": Stall in Brach FU! Cannot move forward!" << endl;
        dirty_latch[DRF] = true;
      }else if (d.ready()){
        next_pc[BEU] = i;
        cout << i <<": All sources are valid! Issuing instruction " << i << " to Branch FU" << endl;
        dirty_latch[BEU] = true;
      }else {
        //where we have a dependency issue.
        // BZ & BNZ - zero flag  JUMP - X register BAL - src 1
        if (di == BZ || di == BNZ){
          if(fwd_val[ALU2][2] == d.depends(1)){
            d.set_src(1, is_zero(fwd_val[ALU2][1]));
            d.mark_as_valid(1);
            cout << i << ": Decode read value forwarded from ALU 2!" << endl;
          } else if(fwd_val[MEM][2] == d.depends(1)){
            d.set_src(1, is_zero(fwd_val[MEM][1]));
            d.mark_as_valid(1);
            cout << i << ": Decode read value forwarded from Memory!" << endl;
          }else if(fwd_val[WB][2] == d.depends(1)){
            d.set_src(1, is_zero(fwd_val[WB][1]));
            d.mark_as_valid(1);
            cout << i << ": Decode read value forwarded from Write Back!" << endl;
          } else {
            //we didn't find it :/
            //stall 
            stall[DRF] = true;
            next_pc[DRF] = i;
            dirty_latch[DRF] = true;
            cout << i << ": Decode could not resolve all sources in time. Cannot move forward" << endl;
          }
        } else if (di == JUMP){
          if(fwd_val[BEU][2] == d.depends(1)){
            d.set_src(1, fwd_val[BEU][1]);
            d.mark_as_valid(1);
            cout << i << ": Decode read value forwarded from Breach FU!" << endl;
          } else if (fwd_val[DELAY][2] == d.depends(1)){
            d.set_src(1, fwd_val[DELAY][1]);
            d.mark_as_valid(1);
            cout << i << ": Decode read value forwarded from Delay!" << endl;
          } else if (fwd_val[MEM][2] == d.depends(1)){
            d.set_src(1, fwd_val[MEM][1]);
            d.mark_as_valid(1);
            cout << i << ": Decode read value forwarded from Memory!" << endl;
          } else if (fwd_val[WB][2] == d.depends(1)){
            d.set_src(1, fwd_val[WB][1]);
            d.mark_as_valid(1);
            cout << i << ": Decode read value forwarded from Write Back!" << endl;
          } else {
            //didn't find it :/
            stall[DRF] = true;
            next_pc[DRF] = i;
            dirty_latch[DRF] = true;
            cout << i << ": Decode could not resolve all sources in time. Cannot move forward" << endl;
          }
        } else if (di == BAL){
          if(fwd_val[ALU2][2] == d.depends(1)){
            d.set_src(1, fwd_val[ALU2][1]);
            d.mark_as_valid(1);
            cout << i << ": Decode read value forwarded from ALU 2!" << endl;
          } else if(fwd_val[MEM][2] == d.depends(1)){
            d.set_src(1, fwd_val[MEM][1]);
            d.mark_as_valid(1);
            cout << i << ": Decode read value forwarded from Memory!" << endl;
          }else if(fwd_val[WB][2] == d.depends(1)){
            d.set_src(1, fwd_val[WB][1]);
            d.mark_as_valid(1);
            cout << i << ": Decode read value forwarded from Write Back!" << endl;
          } else {
            //we didn't find it :/
            //stall 
            stall[DRF] = true;
            next_pc[DRF] = i;
            dirty_latch[DRF] = true;
            cout << i << ": Decode could not resolve all sources in time. Cannot move forward" << endl;
          }
        } else {
          //error case
          
        }
        if(d.ready()){
          next_pc[BEU] = i;
          dirty_latch[BEU] = true;
          cout << i <<": All sources are valid! Issuing instruction " << i << " to Branch FU" << endl;
          next_pc[DRF] = curr_pc[FETCH];
          dirty_latch[DRF] = true;
        }
      }//end if ! stall
      if (!(stall[ALU1])){
        next_pc[ALU1] = ND;
        dirty_latch[ALU1] = true;
      }//end if ! stall

    } else if (di == STORE){
      //store can go onto ALU1 w/o source 1
      //so it needs a special dependency tree 
      if(stall[ALU1]){
        stall[DRF] = true;
        next_pc[DRF] = i;
        dirty_latch[DRF] = true;
        cout << i << ": Stall in ALU 1! Cannot move forward!" << endl;
      } else if(d.is_valid(2)){
        next_pc[ALU1] = i;
        dirty_latch[ALU1] = true;
        cout << i <<": All sources are valid! Issuing instruction " << i << " to ALU 1" << endl;
      } else {
        if (fwd_val[ALU2][2] == d.depends(2)){
           d.set_src(2, fwd_val[ALU2][1]);
           d.mark_as_valid(2);
            cout << i << ": Decode read value forwarded from ALU 2!" << endl;
        } else if (fwd_val[MEM][2] == d.depends(2)){
          d.set_src(2, fwd_val[MEM][1]);
          d.mark_as_valid(2);
          cout << i << ": Decode read value forwarded from Memory!" << endl;
        } else if (fwd_val[WB][2] == d.depends(2)){
          d.set_src(2, fwd_val[WB][1]);
          d.mark_as_valid(2);
          cout << i << ": Decode read value forwarded from Write Back!" << endl;
        } else{
          // didn't find it!
          stall[DRF] = true;
          next_pc[DRF] = i;
          dirty_latch[DRF] = true;
          cout << i << ": Decode could not resolve all sources in time. Cannot move forward" << endl;
        }
      }
      
      if(!(stall[BEU])){
        next_pc[BEU] = ND;
        dirty_latch[BEU] = true;
      }
      
    } else {  //reg-to-reg,, load & halt 
      if (stall[ALU1]){
        stall[DRF] = true;
        next_pc[DRF] = i;
        dirty_latch[DRF] = true;
        cout << i << ": Stall in ALU 1! Cannot move forward!" << endl;
      } else if(d.ready()){
        next_pc[ALU1] = i;
        dirty_latch[ALU1] = true;
        cout << i <<": All sources are valid! Issuing instruction " << i << " to ALU 1" << endl;
        next_pc[DRF] = curr_pc[FETCH];
        dirty_latch[DRF] = true;
      } else {
        for(int p = 1; p <= 2; p ++){
          if (!(d.is_valid(p))){
            if (fwd_val[ALU2][2] == d.depends(p)){
              d.set_src(p, fwd_val[ALU2][1]); 
              d.mark_as_valid(p);
              cout << i << ": Decode read value forwarded from ALU 2!" << endl;
            }else if (fwd_val[MEM][2] == d.depends(p)) {
              d.set_src(p, fwd_val[MEM][1]);
              d.mark_as_valid(p);
              cout << i << ": Decode read value forwarded from Memory!" << endl;
            }else if (fwd_val[WB][2] == d.depends(p)) {
              d.set_src(p, fwd_val[WB][1]);
              d.mark_as_valid(p);
              cout << i << ": Decode read value forwarded from Write Back!" << endl;
            } else {
              //didn't find
              ;
            }//end dependency tree
          }//end p ! valid
        } //end for
      }//end inst not ready
      if (d.ready() && !(stall[ALU1]) && (d.get_int() != BNZ || d.get_int() != BZ || d.get_int() != JUMP || d.get_int() != BAL)){
        next_pc[ALU1] = i;
        dirty_latch[ALU1] = true;
        cout << i <<": All sources are valid! Issuing instruction " << i << " to ALU 1!" << endl;
        next_pc[DRF] = curr_pc[FETCH];
        dirty_latch[DRF] = true;

      } else {
        //didn't find one or more source
        stall[DRF] = true;
        next_pc[DRF] = i;
        dirty_latch[DRF] = true;
        cout << i << ": Decode could not resolve all sources in time." << endl;
      }//end if

      if(!(stall[BEU])){
        next_pc[BEU] = ND;
        dirty_latch[BEU] = true;
      }
    }//end if reg-to-reg

   
  pc_int[i] = d;   
  } else {  // nop 
    cout << "Decode did nothing ..." << endl;
    if (stall[DRF]) stall[DRF] = false;
    if (stall[ALU1]){
      stall[DRF] = true;
      next_pc[DRF] = i;
      dirty_latch[DRF] = true;
    } else {
      next_pc[ALU1] = i;
      dirty_latch[ALU1] = true;
      next_pc[DRF] = curr_pc[FETCH];
      dirty_latch[DRF] = true;
    } //end !stall 
    if(!(stall[BEU])){
      next_pc[BEU] = ND;
      dirty_latch[BEU] = true;
    } //end if !stall beu
  }// end if = ND
  
  return 0;
} // end decode

/**
 * beu
 * Simulates the BRANCH FU stage (the 'branch execution unit').
 */
int beu(int i){
  
  if (stall[BEU]){
    stall[BEU] = false;
  } else {
    if (i != ND){
      bool branch = false;
      Instruction b = pc_int[i];
      switch(b.get_int()){
        case BZ :
          if (b.get_srcs(1)){
            branch = true;
            b.set_res(i + (b.get_lit() / 4));
          }//end if
          break;
        case BNZ :
          if(!(b.get_srcs(1))){
            branch = true;
            b.set_res(i + (b.get_lit() / 4));
          }//end if
          break;
        case JUMP : 
          b.set_res((b.get_srcs(1) + b.get_lit())/4);
          branch = true;
          break;
        case BAL :
          //have to set up stufff;-;
          b.set_res((b.get_srcs(1) + b.get_lit()/4));
          
          branch = true;
          break;
      }//end switch
      if (branch = true){
        cout << i << ": Decided to take a branch! Branch target: " << b.get_res() << endl;
        next_pc[DRF] = ND;
        dirty_latch[DRF] = true;
        next_pc[FETCH] = b.get_res();
        dirty_latch[FETCH] = true;
        squash = true;
      }else{
        squash = false;
        cout << i << ": Decided to not take a branch!" << endl;
      }// end if branch 
      
      if (b.get_int() == BAL){
        b.set_res(i + 1);
        fwd_val[BEU][0] = REG_X;
        fwd_val[BEU][1] = b.get_res();
        fwd_val[BEU][2] = i;
      }
      
    pc_int[i] = b;
    } else { squash = false; cout << "BEU did nothing..." << endl;} // end if ! ND
  }//end ! stall
  if (stall[DELAY]){
    stall[BEU] = true;
    next_pc[BEU] = i;
    dirty_latch[BEU] = true;
  } else {
    next_pc[DELAY] = i;
    dirty_latch[DELAY] = true;
  }//end else stall
  return 0;
}// end beu

/**
 * alu1
 * Simulates the INT. ALU1 stage.
 */
int alu1 (int i){
  if (stall[ALU1]){
    stall[ALU1] = false;
  }else{
      if (i != ND && curr_pc[ALU1] == curr_pc[ALU2]){
        Instruction a = pc_int[i];
        //bitset <12> src1, src2;
        switch (a.get_int()){
          case ADD :
            a.set_res(a.get_srcs(1) + a.get_srcs(2));
            cout << i << ": ALU1 is calculating addition!" << endl;
            break;
          case SUB : 
            a.set_res(a.get_srcs(1) - a.get_srcs(2));
            cout << i << ": ALU1 is calculating subtraction!" << endl;
            break;
          case MUL :
            a.set_res(a.get_srcs(1) * a.get_srcs(2));
            cout << i << ": ALU1 is calculating multiplication!" << endl;
            break;
          case MOVC :
            a.set_res(a.get_lit());
            break;
          case AND :
            a.set_res((int)(bitset<12>(a.get_srcs(1)) & bitset<12>(a.get_srcs(2))).to_ulong());
            cout << i << ": ALU1 is calculating an and!" << endl;
            break;
          case OR :
            a.set_res((int)(bitset<12>(a.get_srcs(1)) | bitset<12>(a.get_srcs(2))).to_ulong());
            cout << i << ": ALU1 is calculating an or!" << endl;
            break;
          case EX_OR :
            a.set_res((int)(bitset<12>(a.get_srcs(1)) ^ bitset<12>(a.get_srcs(2))).to_ulong());
            cout << i << ": ALU1 is calculating! an xor" << endl;
            break;
          case STORE :
            a.set_res(a.get_srcs(2) + a.get_lit());
            cout << i << ": ALU1 is calculating an address offset!" << endl;
            break;
          case LOAD : 
            a.set_res(a.get_srcs(1) + a.get_lit());
            cout << i << ": ALU1 is calculating an address offset!" << endl;
            break;
          default : 
            cout << i << ": Nothing to calculate!" << endl;
            break;
            
      }//end switch
      pc_int[i] = a;
    } else {cout << "ALU 1 did nothing..." << endl;} // if ! ND
  }//end else stall
  if (stall[ALU2]){
    stall[ALU1] = true;
    next_pc[ALU1] = i;
    dirty_latch[ALU1] = true;
  } else {
    next_pc[ALU2] = i;
    dirty_latch[ALU2] = true;
  }
  return 0;
}// end alu1

/**
 * delay
 * Simulates the DELAY stage.
 */
int delay (int i){
  //
  // :( 
  //so like delay does nothing
  //but we might be able to control any conflicts between alu2 and delay here
  //change to the lower pc goes through
  
  //forward BAL case
  if (i != ND){
    if(pc_int[i].get_int() == BAL){
      fwd_val[DELAY][0] = pc_int[i].get_dest();
      fwd_val[DELAY][1] = pc_int[i].get_res();
      fwd_val[DELAY][2] = i;
      cout << i << ": Forwwarding Value from Delay!" << endl;
    } else {
      fwd_val[DELAY][0] = fwd_val[DELAY][1] = fwd_val[DELAY][2] = ND;
    }
    
  } else {
    fwd_val[DELAY][0] = fwd_val[DELAY][1] = fwd_val[DELAY][2] = ND;
  }
  
  
  
  if(i == ND){
    cout << "Delay did nothing..." << endl;
    //allow alu2 to go through
  } else if (stall[MEM]) {
    // i is not ND and there is a stall in MEM
    stall[DELAY] = true;
    next_pc[DELAY] = i;
    dirty_latch[DELAY] = true;
    cout << i << ": Memory is stalled! Cannot move forward!" << endl;
  } else if (stall[ALU2]){
    //i is not ND, there is not stall in MEM and there is a stall in ALU2
    next_pc[MEM] = i;
    dirty_latch[MEM] = true;
    cout << i << ": ALU 2 is stalled! "<< i <<" can move forward!" <<endl;
  } else if (curr_pc[ALU2] == ND){
    //i is not ND, there is no stall in MEM or ALU2, and ALU2 has a NOP
    cout << i<< ": ALU 2 is a NOP! " << i << " can move forward!" << endl;
    next_pc[MEM] = i;
    dirty_latch[MEM] = true;
  } else if (i < curr_pc[ALU2]) {
    //neither i nor ALU2 are ND, there are no stalls in ALU or MEM
    //we let the lower line # go through
    next_pc[MEM] = i;
    dirty_latch[MEM] = true;
    stall[ALU2] = true;
    next_pc[ALU2] = curr_pc[ALU2];
    dirty_latch[ALU2] = true;
    cout << i << ": Delay has a lower line count than ALU 2! Move " << i << " forward. ALU 2 stalls!" << endl;
  } else if (i > curr_pc[ALU2]) {
    //ALU has the lower line number
    stall[DELAY] = true;
    next_pc[DELAY] = i;
    dirty_latch[DELAY] = true;
    cout << i << ": ALU 2 has a lower line count than Delay! Move " << curr_pc[ALU2] << " forward. Delay stalls!" << endl;
  }  else {
    //error case
  }
  
  return 0;
}//end delay

/**
 * alu2
 * Simulates the INT. ALU2 stage.
 */
int alu2(int i){
  //i'm going to program this assuming we already have a value calculated
  if (stall[ALU2]){
    stall[ALU2] = false;
  }else{
    if (i != ND){
      //first we forward the caluculated data
      switch (pc_int[i].get_int()){
        case ADD : case SUB : case MUL : case AND : case OR : case EX_OR : case MOVC :
          cout << i << ": ALU2 finished calculating the result!" << endl;
          fwd_val[ALU2][0] = pc_int[i].get_dest();
          fwd_val[ALU2][1] = pc_int[i].get_res();
          fwd_val[ALU2][2] = i;
          cout << i << ": Forwarded value from ALU 2!" << endl;;
          break;
        default :
          fwd_val[DELAY][0] = fwd_val[DELAY][1] = fwd_val[DELAY][2] = ND;
          break;
      }
    } else {cout << "ALU 2 did nothing..." << endl; fwd_val[DELAY][0] = fwd_val[DELAY][1] = fwd_val[DELAY][2] = ND;}
  }
  //then we deal with STORE
  //STORE is the only instruction that can stall here (out side of you know, there just being a stall somehow)
  //fix
  if (i != ND){
    if (pc_int[i].get_int() == STORE){
      if (pc_int[i].ready()){
        next_pc[MEM] = i;
        dirty_latch[MEM] = true;
      }else {

        if (fwd_val[MEM][0] == pc_int[i].get_dest() && fwd_val[MEM][2] == pc_int[i].depends(1)){
          //search from most recent to furthest
          pc_int[i].set_src(1, fwd_val[MEM][1]);
          pc_int[i].mark_as_valid(1);
          next_pc[MEM] = i;
          dirty_latch[MEM] = true;
          
        }
        else if(rf[pc_int[i].get_src_ar(1)].read_valid_bit()){
          pc_int[i].set_src(1, rf[pc_int[i].get_src_ar(1)].read_value());
          pc_int[i].mark_as_valid(1);
          next_pc[MEM] = i;
          dirty_latch[MEM] = true;
        }
        else {
          //didn't find the value in time
          stall[ALU2] = true;
          next_pc[ALU2] = i;
          dirty_latch[ALU2] = true;
        } 

      }//end if store not ready
    
    } else {

      //not a store
      if (stall[MEM]) {
        //if there's a stall in the next stage & next cycle
        stall[ALU2] = true;
        next_pc[ALU2] = i;
         dirty_latch[ALU2] = true;
      }else {
       next_pc[MEM] = i; 
       dirty_latch[MEM] = true;
      }
    } 
  }//end else not store
  return 0;
}//end alu2

/**
 * mem
 * Simulates the MEM stage (memory access).
 */
int mem(int i){
  if (stall[MEM]){
    stall[MEM] = false;
  } else {
    if(i != ND){
      switch(pc_int[i].get_int()){
        case STORE: 
          write_to_mem(pc_int[i].get_res(), pc_int[i].get_srcs(1)); 
          break;
        case LOAD:
          pc_int[i].set_res(access_memory_at(pc_int[i].get_res()));
          
          break;
      }
      //gots to forward every stage to make sure every guy can get my result!
      //and by every stage I mean ALU2 and MEM
        //jk I need to forward from every stage after decode except alu1
      fwd_val[MEM][0] = pc_int[i].get_dest();
      fwd_val[MEM][1] = pc_int[i].get_res();
      fwd_val[MEM][2] = i;
    } else {cout << "Memory did nothing..." << endl;fwd_val[MEM][0] = fwd_val[MEM][1] = fwd_val[MEM][2] = ND;}
    
    
  }
  //now to process the future!
  if (stall[WB]){
    //since the next stage is always processed before this one
    //it will already know if there is going to be a stall in the next cycle
    stall[MEM] = true;
    next_pc[MEM] = i;
    dirty_latch[MEM] = true;
  } else {
    //since nothing (at least in this sim) can block in mem, these are the only two cases we have to deal with
    next_pc[WB] = i;
    dirty_latch[WB] = true;
  }
  return 0;
}//end mem

/**
 * wb
 * Simulates the WB stage (write-back to register file).
 */
int wb (int i) {
  if(i != ND){
    fwd_val[MEM][0] = pc_int[i].get_dest();
    fwd_val[MEM][1] = pc_int[i].get_res();
    fwd_val[MEM][2] = i;
    if  (pc_int[i].get_dest() != ND) {
      rf[pc_int[i].get_dest()].write_value(pc_int[i].get_res());
      rf[pc_int[i].get_dest()].write_valid_bit(true);
      cout << i << ": WB wrote the result " << pc_int[i].get_res() << " to AR " << pc_int[i].get_dest() << endl;
      }
    if(pc_int[i].get_int() == HALT){
      return EOP;
    }  
  
  } else {cout << "Write Back did nothing..." << endl;fwd_val[WB][0] = fwd_val[WB][1] = fwd_val[WB][2] = ND;}
    
    
  
  return 0;
}
