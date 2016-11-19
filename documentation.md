Stage:
  - each stage is implemented as a function
  - each stage function takes an integer which is the index of the instruction to be processed in that stage
  - each stage function returns an integer referring to whether an error was found in that stage
  - each stage function writes the index of the instruction that will be processed in the next stage to an array representing the program counter chain
  Fetch :
    - Gets the string instruction from the instruction structure
    - Gets the substring from the 1st character to the ' ' and sets the instruction type
    - Checks against special cases (JUMP X & HALT)
    - Retrieves digits from the string and writes them to their appropriate locations
  Decode/RF:
    - Tries to read sources from register file
    - If it cannot read the sources, it sets up a dependency
    - Marks destination register as Invalid
    - Arranged sources and destination not explicitly specified, like in BZ or BAL
    - Tries to retrieve any missing sources from forwarding structure
    - If it cannot, causes a stall
    - Otherwise it issues the instruction to the appropriate FU
    - It also inserts a nop into the FU not in use
  ALU 1:
    - If there is a result to be calculated, it is calculated here
    - In the case of a store, if a source is missing, tries to retrieve missing sources
  ALU 2:
    - Forwards value calculate in ALU 1
    - In case of a store, tries to retrieve any missing source from forwarding structure
    - If it cannot, causes a stall
  BEU:
    - Determines if branch is taken 
    - Sets the 'squash' global variable
    - If branch is taken, arranges the next cycle instructions for fetch and decode
    - In case of a BAL, forwards value
  Delay :
    - In case of a BAL, forwards value
    - Handles conflict between delay and ALU 2 going into memory
  Memory : 
    - In the case of a load or store, read or writes from the memory array as appropriate
    - Forwards result
  Write Back :
    - if there is a destination, writes the result to the destination register
    - forwards result value
    - in the case of a NOP return the end of processing value
Forward:
  - ALU2, Branch FU, Delay, Memory and Write Back write their destination, result and their instruction's program counter to a fowarding structure
  - Decode will detect the register referred to by a source argument, attempt to find it in the register file, and if it cannot, checks the following stages (from ALU2 to Write back) to find the most recently issued instruction that has that source as its destination
  - When decode finds such an instruction, it writes an integer to a forwarding data member in the instruction being decoded, representing the dependency from the decoded instuction to the producer
  - Decode (and ALU1 and ALU2 in the case of a store) checks if a source is valid, and if it is not, it tries to find where the producer wrote to the fowarding structure, and retrieves that value
Stalls: 
  - When a stall is detected in the next instruction in the pipeline, we set stall flag of the current stage to be true, and write the current value of the current stage's program counter to be the 'next instruction' of this stage.
  
  Notes:
  The X register is represented in our simulator as AR 18
