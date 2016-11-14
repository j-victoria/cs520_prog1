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
bool stall [8]; // true if there is a stall in the ith stage
                //only a stage can write to it's own stall
int curr_pc[9];
int next_pc[9]; //need to scrub at every new 

bool squash;  //when true: fetch and decode are not executed 
int z_ptr;    //points to the instrucion that last affected the Z 


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
int main () {
  
  string in;
  
  memset(stall, false, sizeof(stall));
  
  for(int i = 0; i < 9; i++){
    stall[i] = false;
  }
  
  do {
    for (int i = 0; i < 20; i++){
      rf[i].reset();
    }
   
    cout << "Enter a file to execute: ";
    getline(cin, in);
    if (in.empty())
      in = "test_inst";
    cout << "Reading file ..." << cout;
    if (get_inst_from_file(in.c_str()) != 0) {cout << "error opening file" << endl; return 0;}
    cout << "There are " << pc_int.size() << " instructions in " << in  << endl;
    cout << "Starting exectution [enter to go through a cycle]..." << endl;
    memset(next_pc, -1, sizeof(next_pc));
    next_pc[FETCH] = 0;
    do{
      //copy the new pc addresses into the current pc addr buffer
      //acts as latch
      //this is the only time the curr_pc is written to
      memcpy(curr_pc, next_pc, sizeof(curr_pc));
      
      wb(curr_pc[WB]);
      mem(curr_pc[MEM]);
      alu2(curr_pc[ALU2]);
      delay(curr_pc[DELAY]);
      alu1(curr_pc[ALU1]);
      beu(curr_pc[BEU]);
      decode(curr_pc[DRF]);
      fetch(curr_pc[FETCH]);
        
      cin.ignore();
      
    }while (true);
    
    //clear out globals after each run
    //add reamining globals
    pc_int.clear();
    for (int i = 0; i < 20; i++){
      rf[i].reset();
    }
    memset(memory, 0, sizeof(memory));
  }while (true);
  return 0;
}

//bz/bnz will only stall if add/sub/mul in alu1
//change to handle files aside from the 1st
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


int fetch (int inst_index) {
  if (squash) { inst_index = ND;}
  if (!(inst_index == ND)){
    if (stall[FETCH]){
      stall[FETCH] = false;
    } else {
      Instruction i = pc_int[inst_index];
      string s = i.get_name();
      if(s.compare(0, 10, "JUMP X, #0")){
        i.set_inst("JUMP");
        i.set_src(1, REG_X);
        i.set_lit(0);
      }else {
      
        i.set_inst(s.substr(0, s.find(" ")));
        string rest = s.substr(s.find(" "), s.length() - 1);
        int j = 1, r = 1;
        instructions_t inst = i.get_int();

        //set up default case 
        i.set_dest(ND); i.set_src_ar(1, ND); i.set_src_ar(2, ND); i.set_lit(ND); i.set_res(ND);
        //this should make decode slightly easier


        while (j < rest.length()){
          int reg = 0;
          int sign = 1;
          if (rest[j - 1] == '-') sign = -1;

          while ( isdigit( rest[j] ) ) {
            reg = reg * 10 + (int) rest[j];
            j++;
          }
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
          while(!(isdigit(++j)));
        }
      }
    }
  }
  
  
  
  if (stall[DRF]){
    stall[FETCH]= true;
    next_pc[FETCH] = inst_index;
  }else {
    next_pc[DRF] = inst_index;
  }
  
  if (squash){
    squash = false;
  } else {
    next_pc[FETCH] = inst_index + 1;
  }
  
  return 0;
}

int decode (int i){
  
  if(squash) i = ND;
  
  if (i != ND){
    
    
    Instruction d = pc_int[i];
    
    if (stall[DRF]){
      stall[DRF] = false;
    }  else {
      
      d.create_dependency(1, ND); d.create_dependency(2, ND);//assume no dependencies     
      int dep;
      for (int j; j = 1; j < 3){
        if (d.get_src_ar(j) != ND){
          if (rf[d.get_src_ar(j)].read_valid_bit()){
            d.set_src(j, rf[d.get_src_ar(j)].read_value());
            d.mark_as_valid(j);
          } else {
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
            d.create_dependency(j, dep);
            d.mark_as_invalid(j);
          }
        } else {d.set_src(j, ND); d.mark_as_valid(j);}
      }
      if (d.get_dest() != ND){
        //mark destination register as invalid
        rf[d.get_dest()].write_valid_bit(false);
      }
      
      // zero flag stuff!!!
      if (d.get_int() == ADD || d.get_int() == SUB || d.get_int() == MUL){
        z_ptr = i;
      }
      if (d.get_int() == BNZ || d.get_int() == BZ){
        if(z_ptr == curr_pc[ALU1]){  
          d.create_dependency(1, z_ptr);
          d.mark_as_invalid(1);
        } else if (rf[pc_int[z_ptr].get_dest()].read_valid_bit()) {
          d.set_src(1, rf[pc_int[z_ptr].get_dest()].read_z());
          d.mark_as_valid(1);
        } else if(pc_int[curr_pc[ALU2]].get_dest() == z_ptr){
          d.set_src(1, fwd_val[ALU2][1]);
          d.mark_as_valid(1);
        } else if (pc_int[curr_pc[MEM]].get_dest() == z_ptr){
          d.set_src(1, fwd_val[MEM][1]);
          d.mark_as_valid(1);
        }
      }
        
        // X register stuff
        if (d.get_int() == BAL) {
          d.set_dest(REG_X);
          rf[REG_X].write_valid_bit(false);
        } //jump's dependency is taken care of above, since X is just a regular register with a special name :/

      } // end else stall
     
    
    int p;
    
    //issusing 
   intstructions_t di;
    if (di == BAL || di == JUMP || di == BZ || di == BNZ){
      if(stall[BEU]){
        stall[DRF] = true;
        next_pc[DRF] = i;

      }else if (d.ready()){
        next_pc[BEU] = i;

      }else {
        //where we have a dependency issue.
        // BZ & BNZ - zero flag  JUMP - X register BAL - src 1
        if (di == BZ || di == BNZ){
          if(fwd_val[ALU2][2] == d.depends(1)){
            d.set_src(1, is_zero(fwd_val[ALU2][1]));
            d.mark_as_valid(1);
          } else if(fwd_val[MEM][2] == d.depends(1)){
            d.set_src(1, is_zero(fwd_val[MEM][1]));
            d.mark_as_valid(1);
          }else if(fwd_val[WB][2] == d.depends(1)){
            d.set_src(1, is_zero(fwd_val[WB][1]));
            d.mark_as_valid(1);
          } else {
            //we didn't find it :/
            //stall 
            stall[DRF] = true;
            next_pc[DRF] = i;
          }
        } else if (di == JUMP){
          if(fwd_val[BEU][2] == d.depends(1)){
            d.set_src(1, fwd_val[BEU][1]);
            d.mark_as_valid(1);
          } else if (fwd_val[DELAY][2] == d.depends(1)){
            d.set_src(1, fwd_val[DELAY][1]);
            d.mark_as_valid(1);
          } else if (fwd_val[MEM][2] == d.depends(1)){
            d.set_src(1, fwd_val[MEM][1]);
            d.mark_as_valid(1);
          } else if (fwd_val[WB][2] == d.depends(1)){
            d.set_src(1, fwd_val[WB][1]);
            d.mark_as_valid(1);
          } else {
            //didn't find it :/
            stall[DRF] = true;
            next_pc[DRF] = i;
          }
        } else if (di == BAL){
          if(fwd_val[ALU2][2] == d.depends(1)){
            d.set_src(1, fwd_val[ALU2][1]);
            d.mark_as_valid(1);
          } else if(fwd_val[MEM][2] == d.depends(1)){
            d.set_src(1, fwd_val[MEM][1]);
            d.mark_as_valid(1);
          }else if(fwd_val[WB][2] == d.depends(1)){
            d.set_src(1, fwd_val[WB][1]);
            d.mark_as_valid(1);
          } else {
            //we didn't find it :/
            //stall 
            stall[DRF] = true;
            next_pc[DRF] = i;
          }/
        } else {
          //error case
        }
 
      }//end if ! stall
      if (!(stall[ALU1])){
        next_pc[ALU1] = ND;
      }//end if ! stall

    } else if (di == STORE){
      //store can go onto ALU1 w/o source 1
      //so it needs a special dependency tree 
      if(stall[ALU1]){
        stall[DRF] = true;
        next_pc[DRF] = i;
      } else if(d.is_valid(2)){
        next_pc[ALU1] = i;
      } else {
        if (fwd_val[ALU2][2] == d.depends(2)){
           d.set_src(2, fwd_val[ALU2][1]);
           d.mark_as_valid(2);
        } else if (fwd_val[MEM][2] == d.depends(2)){
          d.set_src(2, fwd_val[MEM][1]);
          d.mark_as_valid(2);
        } else if (fwd_val[WB][2] == d.depends(2)){
          d.set_src(2, fwd_val[WB][1]);
          d.mark_as_valid(2);
        } else{
          // didn't find it!
          stall[DRF] = true;
          next_pc[DRF] = i;
        }
      }
      
      if(!(stall[BEU])){
        next_pc[BEU] = ND;
      }
      
    } else {  //reg-to-reg,, load & halt 
      if (stall[ALU1]){
        stall[DRF] = true;
        next_pc[DRF] = i;
      } else if(d.ready()){
        next_pc[ALU1] = i;
      } else {
        for(int p = 1; p <= 2; p ++){
          if (!(d.is_valid(p))){
            if (fwd_val[ALU2][2] == d.depends(p)){
              d.set_src(p, fwd_val[ALU2][1]); 
              d.mark_as_valid(p);
            }else if (fwd_val[MEM][2] == d.depends(p)) {
              d.set_src(p, fwd_val[MEM][1]);
              d.mark_as_valid(p);
            }else if (fwd_val[WB][2] == d.depends(p))) {
              d.set_src(p, fwd_val[WB][1]);
              d.mark_as_valid(p);
            } else {
              //didn't find
            }//end dependency tree
          }
          if (d.ready()){
            next_pc[ALU1] = i;
          } else {
            //didn't find one or more source
            stall[DRF] = true;
            next_pc[DRF] = i;
          }//end if
        }
        if(!(stall[BEU])){
          next_pc[BEU] = ND;
        }
      }//end switch

    }//end ! ND
     
  } else {  // nop 
    if (stall[DRF]) stall[DRF] = false;
    if (stall[ALU1]){
      stall[DRF] = true;
      next_pc[DRF] = i;
    } else {
      next_pc[ALU1] = i;
    } //end !stall 
    if(!(stall[BEU])){
      next_pc[BEU] = ND;
    } //end if !stall beu
  }// end if = ND
} // end decode

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
          b.set_res((b.get_src(1) + b.get_lit())/4);
          branch = true;
          break;
        case BAL :
          //have to set up stufff;-;
          b.set_res((b.get_src(1) + b.get_lit()/4));
          
          branch = true;
          break;
      }//end switch
      if (branch = true){
        next_pc[DRF] = ND;
        next_pc[FETCH] = b.get_res();
        squash = true;
      }else{
        squash = false;
      }// end if branch 
      
      if (b.get_int() == BAL){
        b.set_res(i + 1);
        fwd_val[BEU][0] = REG_X;
        fwd_val[BEU][1] = b.get_res();
        fwd_val[BEU][2] = i;
      }
      
    } else { squash = false;} // end if ! ND
  }//end ! stall
  if (stall[DELAY]){
    stall[BEU] = true;
    next_pc[BEU] = i;
  } else {
    next_pc[DELAY] = i;
  }//end else stall
}// end beu

int alu1 (int i){
  if (stall[ALU1]){
    stall[ALU1] = false;
  }else{
      if (i != ND){
        Instruction a = pc_int[i];
        //bitset <12> src1, src2;
        switch (a.get_int()){
          case ADD :
            a.set_res(a.get_srcs(1) + a.get_srcs(2));
            break;
          case SUB : 
            a.set_res(a.get_srcs(1) - a.get_srcs(2));
            break;
          case MUL :
            a.set_res(a.get_srcs(1) * a.get_srcs(2));
            break;
          case MOVC :
            a.set_res(a.get_lit());
            break;
          case AND :
            a.set_res((int)(bitset<12>(a.get_srcs(1)) & bitset<12>(a.get_srcs(2))).to_ulong());
            break;
          case OR :
            a.set_res((int)(bitset<12>(a.get_srcs(1)) | bitset<12>(a.get_srcs(2))).to_ulong());
            break;
          case EX_OR :
            a.set_res((int)(bitset<12>(a.get_srcs(1)) ^ bitset<12>(a.get_srcs(2))).to_ulong());
            break;
          case STORE :
            a.set_res(a.get_srcs(2) + a.get_lit());
              break;
          case LOAD : 
            a.set_res(a.get_srcs(1) + a.get_lit());
            break;
    
      }//end switch
    } // if ! ND
  }//end else stall
  if (stall[ALU2]){
    stall[ALU1] = true;
    next_pc[ALU1] = i;
  } else {
    next_pc[ALU2] = i;
  }
  
}// end alu1


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
    }
  }
  
  
  
  if(i == ND){
    //allow alu2 to go through
  } else if (stall[MEM]) {
    // i is not ND and there is a stall in MEM
    stall[DELAY] = true;
    next_pc[DELAY] = i;
  } else if (stall[ALU2]){
    //i is not ND, there is not stall in MEM and there is a stall in ALU2
    next_pc[MEM] = i;
  } else if (curr_pc[ALU2] == ND){
    //i is not ND, there is no stall in MEM or ALU2, and ALU2 has a NOP
    next_pc[MEM] = i;
  } else if (i < curr_pc[ALU2]) {
    //neither i nor ALU2 are ND, there are no stalls in ALU or MEM
    //we let the lower line # go through
    next_pc[MEM] = i;
    stall[ALU2] = true;
    next_pc[ALU2] = curr_pc[ALU2];
  } else if (i > curr_pc[ALU2]) {
    //ALU has the lower line number
    stall[DELAY] = true;
    next_pc[DELAY] = i;
  }  else {
    //error case
  }
  
}//end delay


int alu2(int i){
  //i'm going to program this assuming we already have a value calculated
  if (stall[ALU2]){
    stall[ALU2] = false;
  }else{
    if (i != ND){
      //first we forward the caluculated data
      switch (pc_int[i].get_int()){
        case ADD : case SUB : case MUL : case AND : case OR : case EX_OR : case MOVC :
          fwd_val[ALU2][0] = pc_int[i].get_dest();
          fwd_val[ALU2][1] = pc_int[i].get_res();
          fwd_val[ALU2][2] = i;
          break;
        default :
          break;
      }
    }
  }
  //then we deal with STORE
  //STORE is the only instruction that can stall here (out side of you know, there just being a stall somehow)
  //fix
  if (i != ND){
    if (pc_int[i].get_int() == STORE){
      if (pc_int[i].ready()){
        next_pc[MEM] = i;
      }else {

        if (fwd_val[MEM][0] == pc_int[i].get_dest() && fwd_val[MEM][2] == pc_int[i].depends(1)){
          //search from most recent to furthest
          pc_int[i].set_src(1, fwd_val[MEM][1]);
          pc_int[i].mark_as_valid(1);
          next_pc[MEM] = i;
        }
        else if(rf[pc_int[i].get_src_ar(1)].read_valid_bit()){
          pc_int[i].set_src(1, rf[pc_int[i].get_src_ar(1)].read_value());
          pc_int[i].mark_as_valid(1);
          next_pc[MEM] = i;
        }
        else {
          //didn't find the value in time
          stall[ALU2] = true;
          next_pc[ALU2] = i;
        } 

      }//end if store not ready
    }
  }else {
    
    //not a store
    if (stall[MEM]) {
      //if there's a stall in the next stage & next cycle
      stall[ALU2] = true;
      next_pc[ALU2] = i;
    }else {
     next_pc[MEM] = i; 
    }
  } //end else not store
  
  return 0;
}//end alu2


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
    fwd_val[MEM][0] = pc_int[i].get_dest();
    fwd_val[MEM][1] = pc_int[i].get_res();
    fwd_val[MEM][2] = i;
    }
    
    
  }
  //now to process the future!
  if (stall[WB]){
    //since the next stage is always processed before this one
    //it will already know if there is going to be a stall in the next cycle
    stall[MEM] = true;
    next_pc[MEM] = i;
  } else {
    //since nothing (at least in this sim) can block in mem, these are the only two cases we have to deal with
    next_pc[WB] = i;
  }
  return 0;
}

int wb (int i) {
  if(i != ND){
    fwd_val[MEM][0] = pc_int[i].get_dest();
    fwd_val[MEM][1] = pc_int[i].get_res();
    fwd_val[MEM][2] = i;
    if  (pc_int[i].get_dest() != ND) {
      rf[pc_int[i].get_dest()].write_value(pc_int[i].get_res());
      rf[pc_int[i].get_dest()].write_valid_bit(true);
      }
    if(pc_int[i].get_int() == HALT){
      return EOP;
    }  
  
  }
    
    
  
  return 0;
}