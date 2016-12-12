using namespace std;

#include <vector>
#include <fstream>
#include <iostream>
#include <string>
#include <cstdlib>
#include <algorithm>
#include <bitset>
#include <cassert>
#include <queue>
#include <cctype>

//local files
#include "util.h"

#define IQ_SIZE 12
#define ROB_SIZE 40
#define MEM_SIZE 1000
#define URF_SIZE 33

#define REG_X 16
#define ND -500


int urf_s = 32;
vector<Register> urf;
vector<Instruction> icache;
int RAT[17];
int RRAT[17];
iqe IQ[IQ_SIZE];
robe ROB[ROB_SIZE];
int rob_head, rob_tail;
int memory[MEM_SIZE];
priority_queue<int, vector<int>, std::greater<int> > FL;

#define DECODE 0
#define DISPATCH 1
#define ALU1 2
#define ALU2 3
#define ALUWB 4
#define MFU 5
#define MFUWB 6
#define LSU1 7
#define LSU2 8
#define MEM 9
#define LSWB 10
#define BEU 11
#define FETCH 12 


int fetch_c, fetch_n;
Instruction * latch_c[13];
Instruction * latch_n[13];
int dispatch_cycle;
bool b_flag;
bool s_d_flag;  //there is a stall in dispatch
bool s_ls_flag; //there is a stall in load/store alu
int branch_target;
int counter;
string file_name;
Instruction * last_branch;
bool d;
vector<string> imem;
Instruction * z_ptr;
bool eoe_flag;
bool s_ls_1;

int stall_dis, no_is, loads, stores, instructions;

/**
 * fetch
 * Simulates the FETCH stage.
 */
int fetch (Instruction * inst){
  //fills in an empty instruction with the instruction string/name
  //from file (i.e. to simulate from memory)
  if (d) cout << "fetching...\n";
  if (fetch_c == 0){
    ifstream inst_file;
    inst_file.open(file_name.c_str());
    if (d) cout << "opened file...\n";
    if (!(inst_file.is_open())){
      cout << "could not open file \n";
      exit(0);
    }
    string s;
    while(getline(inst_file, s) ){
      if(s.find_first_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ") != string::npos)
      {
        imem.push_back(s);
      }
    }
    inst_file.close();
    for(int i = 0; i < imem.size(); i++){
      if(d) cout << imem[i] << endl;
    }
  }
  if (fetch_c < imem.size()){
    if (d) cout << "found : " << imem[fetch_c] << endl;
    inst->name = imem[fetch_c];
    inst->valid = true;
    inst->index = fetch_c;
    inst->dest= inst->src1_v= inst->src1_a= inst->src2_v= inst->src2_a= inst->lit= inst->res = inst->dest_ar = ND;
    inst->src1 = inst->src2 = true;
		inst->branch_taken = false;
    if (d) cout << "name as : " << inst->name << endl;
  } else {
    inst = NULL;
  }
  latch_c[FETCH] = inst;
    //this time each instruction is responsible for itself
  //
  return 0;
}

/**
 * decode
 * Simulates the D/RF stage.
 */
int decode (Instruction * inst){
  //decode stuff
  // grab sources from the urf
  string s; 
  if (inst != NULL){
    inst->latch_loc = DECODE;
    if ((inst->name)[0] == 'H') {
      inst->type = HALT;
    } else {
      s = (inst->name).substr(0, (inst->name).find(' '));
      
      if (d) cout << "type as : " << s << endl;
      
      if (s == "ADD") inst->type = ADD;
      else if (s == "SUB") inst->type = SUB;
      else if (s == "MOVC") inst->type = MOVC;
      else if (s == "MUL") inst->type = MUL;
      else if (s == "AND") inst->type= AND;
      else if (s == "OR") inst->type = OR;
      else if (s == "EX_OR" || s == "EX-OR") inst->type = EX_OR;
      else if (s == "LOAD") inst->type = LOAD;
      else if (s == "STORE") inst->type = STORE;
      else if (s == "BZ") inst->type = BZ;
      else if (s == "BNZ") inst->type = BNZ;
      else if (s == "JUMP") inst->type = JUMP;
      else if (s == "BAL") inst->type = BAL;
      else if (s == "HALT") inst->type = HALT;
      else {cout << "SOmething went wrong :(\n"; exit(0); }

      if (inst->type == JUMP && inst->name.find('X') != string::npos){
				if (d) cout << "X: " << RAT[REG_X];
        inst->src1_a = RAT[REG_X];
				if (d) cout << "src1: " << inst->src1_a;
				inst->lit = 0;
      }
      int r = 1, j = 0;

      while (j < inst->name.length() - 1 && j != string::npos){
        j = inst->name.find_first_of("0123456789", j + 1);
        int ar = 0; 
        int sign = ((inst->name)[j-1] == '-'? -1 : 1);
        while ( isdigit( inst->name[j] ) ) {
          ar = ar * 10 + ((int) inst->name[j] - 48);
          j++;
        }
        ar = ar * sign;
				if (d) cout << "found number : " << ar << endl;
        if (r == 1){  //this is the first number we came across
          switch(inst->type){
            case ADD : case SUB : case MUL : case AND : case OR : case EX_OR : case MOVC : case LOAD :
              inst->dest_ar = ar;
							
              break;
            case STORE : case BAL :
              inst->src1_a = RAT[ar];
              break;
            case BZ : case BNZ :
              inst->lit = ar;
              break;
            case JUMP :
              if (inst->src1_a != RAT[REG_X]){
                inst->src1_a = RAT[ar];
              } else {
                inst->lit = ar;
              }
              break;
            default:
              cout << "  SOMETHING went wrong :((()))\n"; 
              exit(0);
              break;
          }
        } else if (r == 2){
          switch(inst->type){
            case ADD: case SUB: case MUL: case AND: case OR: case EX_OR: case LOAD:
              inst->src1_a = RAT[ar];
              break;
            case MOVC: case BAL: case JUMP:
              inst->lit = ar;
              break;
            case STORE:
              inst->src2_a = RAT[ar];
              break;
            default:
              cout << "OOPS!!!!! \n"; 
              exit(0);
              break;  
          }
        } else if (r == 3){
          switch(inst->type){
            case ADD: case SUB: case MUL: case AND: case OR: case EX_OR:
              inst->src2_a = RAT[ar];  
              break;
            case LOAD: case STORE:
              inst->lit = ar;
              break;
            default:
              cout << " WHAT THE HECK HAPPENED HERE????\n"; 
              exit(0);
              break;
          }
        } else {
          cout << "U ARE ALLOWED 3  NUMBERS! 3!!!!!\n" << endl;
          exit (0);
        }
        r++;  
      }
    }
    if (inst->type == BAL){
      inst->dest_ar = REG_X;
    } else if (inst->type == BZ || inst->type == BNZ){
      inst->src1_a = z_ptr->dest;
    }
    if (d) cout << inst->name <<endl;
    if (d) cout << "dest_ar: " << inst->dest_ar<< endl;
    if (d) cout << "src1_a: " << inst->src1_a << endl;
    if (d) cout << "src2_a: " << inst->src2_a << endl;
    if (d) cout << "lit: " << inst->lit << endl;
    
    if (inst->src1_a != ND){
      if(urf[inst->src1_a].valid){
        inst->src1_v = urf[inst->src1_a].value;
        inst->src1 = true;
        if (d) cout << "found source 1 value in register: " << inst->src1_v <<endl;
      } else {
        urf[inst->src1_a].consumer1.push_back(inst);
        inst->src1 =false;
        if (d) cout << "could not find value for source 1 in register file \n";
      }
    }
    if (inst->src2_a != ND){
      if(urf[inst->src2_a].valid){
        inst->src2_v = urf[inst->src2_a].value;
        inst->src2 = true;
        if (d) cout << "found source 2 value in register: " << inst->src2_v <<endl;
      } else {
        urf[inst->src2_a].consumer2.push_back(inst);
        inst->src2 = false;
        if (d) cout << "could not find value for source 2 in register file \n";
      }
    }
  }
  return 0;
}

int dispatch (Instruction * inst){
  //set up IQ entry
  //mark stuff as 
  //set up rat due to dest
  s_d_flag = false;
  if (inst != NULL){
    inst->latch_loc = DISPATCH;
    
    //check registers again (don't ask :/)
    
    if (!inst->src1){
      if (urf[inst->src1_a].valid){
        inst->src1_v = urf[inst->src1_a].value;
        inst->src1 = true;
      }
    }
    if (!inst->src2){
      if (urf[inst->src2_a].valid){
        inst->src2_v = urf[inst->src2_a].value;
        inst->src2 = true;
      }
    }
    int i = 0;
    while ((IQ[i].valid) && i < IQ_SIZE) i++;
    
    if((inst->dest_ar == ND || !FL.empty()) && !IQ[i].valid && !ROB[rob_tail].valid && (last_branch == NULL || !(inst->type == BAL || inst->type == JUMP || inst->type == BZ || inst->type == BNZ))){
      //dispatch!
      if (d) cout << "dispatching...\n";
			
      int rn;
      if (inst->dest_ar != ND){
        if (d) cout << "setting up RAT \n";
        rn = RAT[inst->dest_ar];
        inst->dest = RAT[inst->dest_ar] = FL.top();
        FL.pop();
        urf[inst->dest].producer = inst;
        urf[inst->dest].consumer1.clear();
        urf[inst->dest].consumer1.push_back(NULL);
        urf[inst->dest].consumer2.clear();
        urf[inst->dest].consumer2.push_back(NULL); 
      }
      if (d) cout << "setting up IQ entry\n";
      IQ[i].inst = inst;
      IQ[i].valid = true;
      inst->iq_loc = i;
      inst->in_iq = true;
      
      if (d) cout << "setting up ROB entry\n";
      ROB[rob_tail].inst = inst;
      ROB[rob_tail].valid = true;
			ROB[rob_tail].finished = false;
      if (inst->dest_ar != ND) {
				ROB[rob_tail].renaming = rn;
			} else {
				ROB[rob_tail].renaming = ND;
			}
      inst->rob_loc = rob_tail;
      rob_tail = (rob_tail + 1) % ROB_SIZE;
      if (inst->type == HALT) ROB[inst->rob_loc].finished = true;
				
      if (d) cout << "setting up other things\n";
      inst->dc = dispatch_cycle;
      if (inst->type == BAL || inst->type == JUMP || inst->type == BZ || inst->type == BNZ){
        last_branch = inst;
      } else if (inst->type == ADD || inst->type == SUB || inst->type == MUL){
        z_ptr = inst;
      }
    }else {
      //stall!
      s_d_flag = true;
      if (d) cout << "could not move on for some reason...\n" << endl;
    }
  }
  return 0;
}

//this is where the IQ is
//decoupled

int alu1 (){
  Instruction * inst = NULL;
  int min = 1000000;
  int index;
  //linearly probe the IQ for the entry we want
  for(int i = 0; i < IQ_SIZE; i++){
    if (IQ[i].valid){
      if (IQ[i].inst->src1 && IQ[i].inst->src2){
        if (IQ[i].inst->type == ADD || IQ[i].inst->type == SUB || IQ[i].inst->type == AND || IQ[i].inst->type == OR || IQ[i].inst->type == EX_OR || IQ[i].inst->type == MOVC){
          if(IQ[i].inst->dc < min){
            if (d) cout << "found candidate: " << IQ[i].inst->name << endl;
            min = IQ[i].inst->dc;
            inst = IQ[i].inst;
            index = i;
          }
        }
      }
    }
  }
  latch_c[ALU1] = inst;
  if (!(inst == NULL)){
    if(d) cout << "alu1 instruction: " << inst->name << endl;
    inst->latch_loc = ALU1;
    IQ[index].valid = false;
    inst->in_iq = false;
  }else {
    if(d) cout << "alu1: no candidate found\n";
  } 
  
  return 0;
}

int alu2 (Instruction * inst){
  //actually calculate results lol
  //fwd results
  if (inst != NULL){
    inst->latch_loc = ALU2;
    switch(inst->type){
      case ADD:
        inst->res = inst->src1_v + inst->src2_v;
        break;
      case SUB:
        inst->res = inst->src1_v - inst->src2_v;
        break;
      case AND:
        inst->res = (int)(bitset<12>(inst->src1_v) & bitset<12>(inst->src2_v)).to_ulong();
        break;
      case OR:
        inst->res = (int)(bitset<12>(inst->src1_v) | bitset<12>(inst->src2_v)).to_ulong();
        break;
      case EX_OR:
        inst->res = (int)(bitset<12>(inst->src1_v) ^ bitset<12>(inst->src2_v)).to_ulong();
        break;
      case MOVC:
        inst->res = inst->lit;
        break;
      default:
        cout << "Something went wrong :/" << endl;
        exit(0);
        break;
    }
    while(!urf[inst->dest].consumer1.empty() && urf[inst->dest].consumer1.back() != NULL){
      Instruction * con = urf[inst->dest].consumer1.back();
      if (!(con->src1)){
        con->src1_v = inst->res;
        con->src1 = true;
        if (d) cout <<"forwarded result " << inst->res << " to " << con->name <<endl;
        urf[inst->dest].consumer1.pop_back();
      }
    }
		if (d) cout << "finished fwd 1\n";
		if (d) cout << urf[inst->dest].consumer2.size();
		
    while(!urf[inst->dest].consumer2.empty() && urf[inst->dest].consumer2.back() != NULL && urf[inst->dest].consumer2.size() > 1){
      Instruction * con = urf[inst->dest].consumer2.back();
      if (!(con->src2)){
        con->src2_v = inst->res;
        con->src2 = true;
        if (d) cout <<"forwarded result " << inst->res << " to " << con->name <<endl;
        urf[inst->dest].consumer2.pop_back();
      }
			
    }
		if (d) cout << "finished fwd 2\n";
  	if (d) cout << "alu2 calculated result for: " << inst->name << ": " << inst->res << endl;
  }
  return 0;
}


int aluwb (Instruction * inst){
  //write back!
  if (inst != NULL){
    urf[inst->dest].value = inst->res;
    urf[inst->dest].valid = true;
    
    ROB[inst->rob_loc].finished = true;
    if (d) cout << "wrote result to register for " << inst->name << endl;
  }
  return 0;
}

int mfu (Instruction * inst){
  //takes 4 cycles
  //calcuate & fwd results
	if (d) cout << counter << endl;
  if (counter == 0){
    int min = 10000000;
    int index;
    for (int i = 0; i < IQ_SIZE; i ++){
      if(IQ[i].valid){
        if (IQ[i].inst->src1 && IQ[i].inst->src2 && IQ[i].inst->type == MUL && IQ[i].inst->dc < min){
          inst = IQ[i].inst;
          min = inst->dc;
          index = i;
          if (d) cout << "found a candidate for mfu: " << inst->name << endl;
        }
      }
    }
    if (inst != NULL){
      if (d) cout << "instruction found!(mfu): " << inst->name << endl; 
      inst->latch_loc = MFU;
      inst->in_iq = false;
      IQ[index].valid = false;
      counter++;
			latch_c[MFU] = inst;
    }else {
      if (d) cout << "not instruction found (mfu) \n"; 
    }
    
  } else if (counter == 3){
    inst->res = inst->src1_v * inst->src2_v;
    if (d) cout << "mfu found a result for " << inst->name << ": " << inst->res << endl;
    while(!urf[inst->dest].consumer1.empty()&& urf[inst->dest].consumer1.back() != NULL){
      Instruction * con = urf[inst->dest].consumer1.back();
      if (!(con->src1)){
        con->src1_v = inst->res;
        con->src1 = true;
        if (d) cout <<"forwarded result " << inst->res << " to " << con->name <<endl;
        urf[inst->dest].consumer1.pop_back();
      }
    }
    while(!urf[inst->dest].consumer2.empty()&& urf[inst->dest].consumer2.back() != NULL){
      Instruction * con = urf[inst->dest].consumer2.back();
      if (!(con->src2)){
        con->src2_v = inst->res;
        con->src2 = true;
        if (d) cout <<"forwarded result " << inst->res << " to " << con->name <<endl;
        urf[inst->dest].consumer2.pop_back();
      }
    }
   counter = 0;
  } else {
    counter++;
    if (d) cout << "mfu has not yet found a result for: " << inst->name << endl;
  }
  return 0;
}

int mfuwb (Instruction * inst){
  if (inst != NULL){
    inst->latch_loc = MFUWB;
    urf[inst->dest].value = inst->res;
    urf[inst->dest].valid = true;
    ROB[inst->rob_loc].finished = true;
    if (d) cout << "wrote "<< inst->res << " to " << inst->dest << ": "<< inst->name << endl;
  }
  return 0;
}

int lsu1(){
  Instruction * inst = NULL;
  int min = 10000000;
  int index;
  if(d) cout << "searching for an instruction for lsu1 \n";
  for (int i = 0; i < IQ_SIZE; i ++){
    if(IQ[i].valid){
      if ((IQ[i].inst->type == LOAD || IQ[i].inst->type == STORE) && IQ[i].inst->dc < min){
				
        inst = IQ[i].inst;
        min = inst->dc;
        index = i;
        if (d) cout << " candidate found for lsu1: " << inst->name << endl;
      }
    }
  }
	if (inst!= NULL){
		if (!((inst->type == LOAD && inst->src1 && inst->src2) || (inst->type == STORE && inst->src2) )){
			inst = NULL;
		}
	}
	latch_c[LSU1] = inst;
  if (inst != NULL){
    inst->latch_loc = LSU1;
    inst->in_iq = false;
    IQ[index].valid = false;
    if (d) cout << "instruction found (lsu 1): " << inst->name << endl;
    inst->res = (inst->type == LOAD ? inst->src1_v+inst->lit : inst->src2_v+inst->lit);
    if (d) cout << " address calculated for lsu1: " << inst->res << ": " << inst->name <<endl;
		if (inst->type == STORE && !inst->src1) s_ls_1 = true;
  }
  return 0;
}
int lsu2(Instruction * inst){
  s_ls_flag = false;
  if (inst != NULL){
    
    inst->latch_loc = LSU2;
    if (inst->rob_loc == rob_head){
      if (inst->type == LOAD){
        inst->res = memory[inst->res/4];
        if (d) cout << "data retrived from memory: " << inst->res << ": " << inst->name << endl;
        while(!urf[inst->dest].consumer1.empty() && urf[inst->dest].consumer1.back() != NULL){
          //Instruction * con = urf[inst->dest].consumer1.back();
          if (!(urf[inst->dest].consumer1.back()->src1)){
            urf[inst->dest].consumer1.back()->src1_v = inst->res;
            urf[inst->dest].consumer1.back()->src1 = true;
            if (d) cout <<"forwarded result " << inst->res << " to " << urf[inst->dest].consumer1.back()->name <<endl;
            urf[inst->dest].consumer1.pop_back();
          }
          if (d) cout << inst << endl;
        }
        if (d) cout << "1 " << inst << endl;
        if (d) cout << "2 " << inst->dest << endl;
        if (d) cout << "3 " << &urf[inst->dest] << endl;
        if (d) cout << "4 " << urf[inst->dest].value << endl;
        if (d) cout << "5 " << urf[inst->dest].consumer2.size() << endl;
        while(!urf[inst->dest].consumer2.empty() && urf[inst->dest].consumer2.back() != NULL){
          if (d) cout << "1";
          Instruction * con = urf[inst->dest].consumer2.back();
          if (d) cout << "2";
          if (!(con->src2)){
            if (d) cout << "3";
            con->src2_v = inst->res;
            if (d) cout << "4\n";
            con->src2 = true;
            if (d) cout <<"forwarded result " << inst->res << " to " << con->name <<endl;
            urf[inst->dest].consumer2.pop_back();
          }
        }
        if (d) cout <<"done forwarding2" << endl;
      }else if (inst->type == STORE){
        memory[inst->res/4] = inst->src1_v;
        if (d) cout << "data stored "<< inst->src1_v << " at " << inst->res << ": "<< inst->name <<endl;
      }else {
        cout << "??? DANGER WILL ROBINSON!\n";
        exit(0);
      }
    
    } else {
      s_ls_flag = true;
      if (d) cout << "Instruction " << inst->name << "stalled, as it is not the head of the rob"  << inst->rob_loc << " | " << rob_head << endl;
    }
  }
  return 0;
}

int lswb(Instruction * inst){
  if (inst != NULL){
    inst->latch_loc = LSWB;
    if (inst->type == LOAD){
      urf[inst->dest].value = inst->res;
      urf[inst->dest].valid = true;
      
      if (d) cout << inst->name <<" wrote to memory value " << inst->res << " to " << inst->dest;
    }
    ROB[inst->rob_loc].finished = true;
  }
  return 0;
}
int beu(){
  int min = 100000;
  Instruction * inst = NULL;
  int index;
  if (d) cout << "searching for an instruction for BEU\n";
  for (int i = 0; i < IQ_SIZE; i++){
    if (IQ[i].valid){
			if (d) cout << "found a valid entry\n";
      if (IQ[i].inst->src1 && IQ[i].inst->src2 && (IQ[i].inst->type == BAL || IQ[i].inst->type == BZ || IQ[i].inst->type == BNZ || IQ[i].inst->type == JUMP) && IQ[i].inst->dc < min)
      {
        if(d) cout << "found a possible candidate: " << IQ[i].inst->name << endl;
        inst = IQ[i].inst;
        min = inst->dc;
        index = i;
      }
    }
	}
	if (d) cout << "1\n";
  latch_c[BEU] = inst;
	if (d) cout << latch_c[BEU] << "2\n";
  if (inst != NULL){
		
    last_branch = NULL;
    inst->in_iq = false;
    IQ[index].valid = false;
  	
    switch(inst->type){
      case BZ:
        if (inst->src1_v == 0) {
					b_flag = true; 
					inst->branch_taken = true;
				}
        branch_target = inst->res = inst->index + (inst->lit / 4);
				if (d) cout << "branch target calculated to be: " << inst->res<< endl;
        break;
      case BNZ:
        if (inst->src1_v != 0 ){
					b_flag = true; 
					inst->branch_taken = true;
				}
        branch_target = inst->res = inst->index + (inst->lit / 4);
				if (d) cout << "branch target calculated to be: " << inst->res<< endl;
        break;
      case BAL: case JUMP :
        b_flag = true; inst->branch_taken = true;
        branch_target = inst->res = (inst->src1_v + inst->lit - 4000) / 4;
				if (d) cout << "branch target calculated to be: " << inst->res<< endl;
        break;
      default:
        exit(0);
    }

    if (inst -> type == BAL){
      urf[inst->dest].value = (inst->index + 1) * 4 + 4000;
      urf[inst->dest].valid = true;
			if (d) cout << inst->name <<" wrote "<<  inst->index + 1 << " to " << inst->dest << endl;
      //we actually don't have to forward b/c of how the branches are set up
    }

    ROB[inst->rob_loc].finished = true;
  } else {
    if (d) cout << "could not find a ready instruction\n";
  }
  return 0;
}

int simulate(){
   
  if (!eoe_flag){
   	//cout << "simulating...\n";
    int rv;

    
    //lswb
    if (! s_ls_flag){
      latch_c[LSWB] = latch_c[LSU2];
    } else {
      latch_c[LSWB] = NULL;
    }
    rv = lswb(latch_c[LSWB]);
    assert(rv == 0);

    if (d) cout << "l/s write back completed \n";

    //ls2 & ls1
    if (!s_ls_flag && !s_ls_1){
      latch_c[LSU2] = latch_c[LSU1];
      rv = lsu2(latch_c[LSU2]);
      assert(rv ==0);
    } else if (latch_c[LSU1] != NULL && !s_ls_flag){
			if (latch_c[LSU1]->src1){
				s_ls_1 = false;
				latch_c[LSU2] = latch_c[LSU1];
				lsu2(latch_c[LSU2]);
			}
		}else{
      rv = lsu2(latch_c[LSU2]);
      assert(rv == 0);
      if (d) cout << "done with ls2\n";
      
    }
		//aluwb
    latch_c[ALUWB] = latch_c[ALU2];
    aluwb(latch_c[ALUWB]);

    //alu2
    latch_c[ALU2] = latch_c[ALU1];
    alu2(latch_c[ALU2]);
	

    //mwb
    if (counter == 0){
      latch_c[MFUWB] = latch_c[MFU];
      rv = mfuwb(latch_c[MFUWB]);
      assert(rv == 0);
      
			latch_c[MFU] = NULL;
			rv = mfu(latch_c[MFU]);
			assert(rv==0);
		
    } else {
			rv = mfu(latch_c[MFU]);
			assert(rv==0);
		}

    
    //fetchey stuff
    alu1();
		//beu
    rv = beu(); 
    assert(rv == 0);
		
		
		if (!s_ls_flag && !s_ls_1){
			rv = lsu1();
      assert(rv == 0);
		} else if (latch_c[LSU1] == NULL){
			if (!s_ls_1){
				rv = lsu1();
        assert(rv == 0);
			}
		}	
		if (latch_c[ALU1] == NULL && (latch_c[LSU1] == NULL) && latch_c[BEU] == NULL && counter != 1) no_is++;
    //dispatch & decode & fetch
    Instruction f;
    if (b_flag){
      //do nothing 
			stall_dis ++;
			if (d) cout << "nothing is happening because a branch is gonna come in here and fuck every thing up\n";
      
    } else if (s_d_flag){
      rv = dispatch(latch_c[DISPATCH]);
      assert(rv == 0);
			stall_dis++;
    }else {
      fetch_c = fetch_c + 1;
      latch_c[DISPATCH] = latch_c[DECODE];
      latch_c[DECODE] = latch_c[FETCH];
      rv = dispatch(latch_c[DISPATCH]);
      assert(rv ==0);
      rv = decode(latch_c[DECODE]);
      assert(rv ==0);
      if (d && latch_c[DECODE]!= NULL) cout << "decode decoded: " << latch_c[DECODE]->name << endl;
      rv = fetch(&f);
      if (latch_c[FETCH] != NULL){
        icache.push_back(f);
        latch_c[FETCH] = &icache.back();
        if (d) cout << "fetch found: " << latch_c[FETCH]->name << endl; 
      }
    }

    if (d) cout << "dispatch, decode & fetch completed \n";

    //retirement  
    if (ROB[rob_head].valid && ROB[rob_head].finished){
			instructions++;
      if (d) cout << "retiring: " << ROB[rob_head].inst->name << " at " << rob_head << endl;
      if (ROB[rob_head].inst->type == HALT){
				eoe_flag = true;
				cout << "execution is done\n";
				for(int i = 0 ; i < 17; i++){
					RAT[i] = RRAT[i];
				}
			} 
			if (ROB[rob_head].inst->type == STORE) stores ++;
			if (ROB[rob_head].inst->type == LOAD) loads ++;
      if (ROB[rob_head].renaming!= ND){
        urf[ROB[rob_head].renaming].valid = false;
        FL.push(ROB[rob_head].renaming);
        if (d) cout << "freeing PR" << ROB[rob_head].renaming << endl;
				
      }
      if (ROB[rob_head].inst->dest != ND){
				RRAT[ROB[rob_head].inst->dest_ar] = ROB[rob_head].inst->dest;
			}
      if ((ROB[rob_head].inst->type == BAL || ROB[rob_head].inst->type == JUMP || ROB[rob_head].inst->type == BZ || ROB[rob_head].inst->type == BNZ) && ROB[rob_head].inst->branch_taken){
				if(d) cout << "ITS A BRANCH TAKE LETS FUCK IT ALL UP\n";
				for (int i = 0; i < IQ_SIZE; i++){
					IQ[i].valid = false;
				}
				for (int i = 0; i <= FETCH; i++){
					latch_c[i] = NULL;
				}
				for (int i = 0; i <= REG_X; i++){
					if (RAT[i] != RRAT[i]){
						urf[RAT[i]].valid = false;
						if (d) cout << "freeing " << RAT[i] << endl;
						FL.push(RAT[i]);
						RAT[i] = RRAT[i];
					}
				}
				for (int i = 0; i < ROB_SIZE; i++){
					ROB[i].valid = false;
				}
				b_flag = false;
				fetch_c = branch_target - 1;
				ROB[rob_head].valid = false;
				rob_head = rob_tail = 0;
			} else {
				ROB[rob_head].valid = false;
      	rob_head = (rob_head + 1) % ROB_SIZE;
			}
    }
  dispatch_cycle ++;
  
  //cout << "end of cycle: " << dispatch_cycle <<endl;
	} else {
    
  }
  return 0;
}

void init (){
	urf.resize(urf_s);
	while (!FL.empty()) FL.pop();
  for (int i = 0; i < urf_s; i++){
    urf[i].value = 0;
    urf[i].valid = false;
    urf[i].consumer1.resize(0);
    urf[i].consumer2.resize(0);
    
    FL.push(i);
  }
  for (int i = 0; i < IQ_SIZE; i ++){
    IQ[i].valid = false;
  }
  for (int i = 0; i < ROB_SIZE; i ++){
    ROB[i].valid = false;
		ROB[i].renaming = ND;
  }
  rob_head = rob_tail = 0;
  for (int i = 0; i < MEM_SIZE; i ++){
  memory[i] = 0;
  } 
  fetch_c = fetch_n = -1;
  for (int i = 0; i <= FETCH; i++){
    latch_c[i] = NULL;
  }
  for (int i = 0; i < 17; i++){
    RAT[i] = RRAT[i] = ND;
  }

  eoe_flag = false;
  icache.reserve(50000);
  dispatch_cycle = 0;
  b_flag = false;
  s_d_flag = false; 
  s_ls_flag = false;
  branch_target = 0;
  counter = 0;
  last_branch = NULL;
	instructions = stall_dis =  no_is = loads = stores = 0;
	s_ls_1 = false;
}

/**
 * display
 * Prints the status of each pipeline stage and each architectural register.
 */
void display () {
  cout << "Fetch: " << fetch_c;
  cout << "Decode: " << (latch_c[DECODE] == NULL? "nothing." : latch_c[DECODE]->name) << endl;
  cout << "Dispatch: " << (latch_c[DISPATCH] == NULL? "nothing." : latch_c[DISPATCH]->name);
  cout << "ALU1: " << (latch_c[ALU1] == NULL? "nothing." : latch_c[ALU1]->name);
  cout << "ALU2: " << (latch_c[ALU2] == NULL? "nothing." : latch_c[ALU2]->name);
  cout << "ALU Write Back: " << (latch_c[ALUWB] == NULL? "nothing." : latch_c[ALUWB]->name) << endl;
  cout << "Multiply FU: " << (latch_c[MFU] == NULL? "nothing." : latch_c[MFU]->name);
  cout << "Multiply WB: " << (latch_c[MFUWB] == NULL? "nothing." : latch_c[MFUWB]->name);
  cout << "L/S FU 1: " << (latch_c[LSU1] == NULL? "nothing." : latch_c[LSU1]->name) << endl;
  cout << "L/S FU 2: " << (latch_c[LSU2] == NULL? "nothing." : latch_c[LSU2]->name) ;
  cout << "L/S WB: " << (latch_c[LSWB] == NULL? "nothing." : latch_c[LSWB]->name);
  cout << "Branch FU: " << (latch_c[BEU] == NULL? "nothing." : latch_c[BEU]->name) << endl;
  for (int i = 0; i < 17; i++){
    cout << "AR " << i << ": ";
		if (RAT[i] != ND){
			if (urf[RAT[i]].valid){
				cout << urf[RAT[i]].value;
			} else {
				cout << "i";
			}
		} else {
			cout << "i";
		}
		cout << " " ;
    if (i % 4 == 3) cout << endl;
  }
	cout << "\n memory: ";
	for (int i = 0; i < 100; i++){
		cout << i * 4 << ": " << memory[i] << " ";
		if (i % 8 == 7) cout << endl;
	}
}

void print_iq (){
  for(int i = 0; i < IQ_SIZE; i++){
    if (IQ[i].valid){
      cout << i << ": " << IQ[i].inst->name;
      cout << " src1: ";
      if (IQ[i].inst->src1_a == ND){
        cout << "n/a ";
      } else if (!IQ[i].inst->src1) {
        cout << "i ";
      } else {
        cout << "v " <<  IQ[i].inst->src1_v << " ";
      }
      cout << "src2: ";
      if (IQ[i].inst->src2_a == ND){
        cout << "n/a ";
      } else if (!IQ[i].inst->src2) {
        cout << "i ";
      } else {
        cout << "v " <<  IQ[i].inst->src2_v << " ";
      }
      cout << "lit: ";
      if (IQ[i].inst->lit == ND){
        cout << "n/a";
      } else {
        cout << IQ[i].inst->lit;
      }
    } else {
      cout << i  << ": i";
    }
  cout << endl;
  }
}

/**
  * print_memory
  * Prints contents of memory addresses a1 through a2, inclusive.
  * Assumes a1, a2 located on 4 byte boundaries.
  */
void print_memory(int a1, int a2) {
  if(a1 < a2 && a2 < MEM_SIZE * 4) {
    for(int i = a1; i <= a2; i+= 4) {
      cout << "Memory at address " << i << ": " << memory[i / 4] << endl;
    }
  } else {
    cout << "Invalid input to print_memory" << endl;
  }
}

void print_rob(){
	int i = rob_head; 
	cout << "head: " << rob_head << " tail: " << rob_tail << endl;
	while (ROB[i].valid){
		cout << i << ": " << ROB[i].inst->name << endl;
		i = (i + 1) % ROB_SIZE;
	}
	return;
}

void print_map_tables(){
	for (int i = 0; i < 17; i++){
		cout << i << ": RAT ";
		if (RAT[i] != ND){
			cout << RAT[i];
		} else{
			cout << "i";
		}
		cout << " RRAT ";
		if (RRAT[i] != ND){
			cout << RRAT[i] << " ";
		}else {
			cout << "i ";
		}
		if (i%3 == 2 ) cout << endl;
	}
	
}

void print_urf(){
	for (int i = 0; i < urf_s; i++){
		cout << "P" << i << ": ";
		cout << urf[i].value << " ";
		if (urf[i].valid){
			
			if (distance(RRAT, find(RRAT, RRAT+17, i)) != 17){
				cout << "c ";
			} else if (distance(RAT, find(RAT, RAT+17, i)) != 17){
				cout << "a ";
			} else {
				cout << "f ";
			}
		}else {
			cout << "f ";
		}
		if (i % 5 == 4) cout << endl;
	}
	return;
}

void print_stats(){
	cout << "IPC: " << ((float)instructions)/dispatch_cycle << endl;
	cout << "dispatch stalls: "<< stall_dis << endl;
	cout << "cycles where nothing was issued: " << no_is << endl;
	cout << "loads commited: " << loads << endl;
	cout << "stores commited: " << stores<< endl;
	return;
}

/**
 * Main method.
 * Takes one argument (filename of instruction text file).
 */
int main (int argc, char *argv[]) {
  string input;
  file_name = argv[1];
  d = (argc > 2 ? true :false);
  init();
  if (d) cout << "debug mode\n";
  do {
    cout << "what do you want to do?>";
    getline(cin, input);
    //string input_t;
    //input_t.resize(input.length());
    //transform(input.begin(), input.end(), input_t.begin(), toupper);
    for (int i = 0; i < input.length(); i++) input[i] = toupper(input[i]);
    if (d && IQ[0].valid) cout << IQ[0].inst->name << endl;
    if ((input[0] == 's' || input[0] == 'S' )&& input[1]!='E'){
      if (input.find_first_of("0987654321", 1) != string::npos){
      int j = stoi(input.substr(input.find_first_of("0987654321", 1), input.length()), nullptr, 10);
        for (int i = 0; i < j; i++){
          simulate();
        }
				display();
				print_iq();
				print_rob();
				print_urf();
				print_map_tables();
				print_stats();
      } else {
        simulate();
      }
		}else if (input[0] == 'i' || input[0] == 'I'){
			init();
    } else if (input[0] == 'd' || input[0] == 'D'){
      display();
    } else if (input == "PRINT_IQ"){
      print_iq();
    //} else if (strtok(input, " ") == "PRINT_MEMORY") {
      // make print_memory work with 2 integer arguments
      //int a1_in = atoi(strtok(NULL, " "));
      //int a2_in = atoi(strtok(NULL, " "));
      //print_memory(a1_in, a2_in);
		} else if (input == "PRINT_ROB"){
			print_rob();
		}else if (input == "PRINT_URF"){
			print_urf();
		}else if (input == "PRINT_MAP_TABLES"){
			print_map_tables();
		} else if (input.find("SET_URF_SIZE") != string::npos){
			while (!FL.empty()) FL.pop();
			urf_s = stoi(input.substr(input.find_first_of("0987654321", 1), input.length()), nullptr, 10);
			urf.resize(urf_s);
			for (int i = 0; i < urf_s; i++){
				urf[i].value = 0;
				urf[i].valid = false;
				urf[i].consumer1.resize(0);
				urf[i].consumer2.resize(0);
				FL.push(i);
			}
		} else if (input == "PRINT_STATS") {
			print_stats();
		} else if (input[0] == 'Q'){
			icache.clear();
			urf.clear();
			return 0;
		} else {
      // nothing here yet
    }
    
  }while(true);
  
  icache.clear();
	urf.clear();
	return 0;
}
