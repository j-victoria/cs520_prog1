Stage:
  - each stage is implimented as a function
  - every stage/function takes an integer which is the index of instruction to be processed in that stage
  - every stage returns an integer refering to whether an error was found in that stage
  - each stage writes to a strucuture the index of the instruction that will be processed in the next stage
  Fetch :
    - Gets the string instruction from the instruction structure
    - Gets the substring from the 1st character to the ' ' and sets the instruction type
    - Checks against spacial cases (JUMP X & HALT)
    - Retrieves digits from the string and writes them to their appropriate locations
  Decode/RF:
    - Tries to read sources from register file
    - IF it cannot read the sources it sets up a dependency
    - Marks destination register as Invalid
    - Arranged sources and destination not explicitly specified, like in BZ or BAL
    - Tries to retrieve any missing sources from forwarding structure
    - if it cannot, causes a stall
    - Otherwise it issues the instruction to the appropriate FU
    - it also inserts a nop into the other FU
  ALU 1:
    - if there is a result to be calculated, it is calculated here
    - In the case of a store, if a source is missing, tries to retrieve missing sources
  ALU 2:
    - forwards value calculate in ALU 1
    - in the case of a store, tries to retrieve any missing source from forwarding structure
    - if it cannot, causes a stall
  BEU:
    - determines if branch is taken 
    - sets the 'squash' global variable
    - if branch is taken, arranges the next cycle instructions for fetch and decode
    - in the case of a BAL, forwards value
  Delay :
    - in the case of BAL, forwards value
    - handles conflict between delay and ALU 2 going into memory
  MEmory : 
    - in the case of a load or store, read or writes from the memory array as appropriate
    - forwards result
  Write Back :
    - if there is a destination, writes the result to the destination register
    - forwards result value
    - in the case of a NOP return the end of processing value
Forward:
  - ALU2, Branch FU, Delay, Memory and Wriback write their destination, result and their instruction's program counter to a fowarding structure
  - Decode will detect a source argument, try and find it in the register file, and, if it cannot, checks stages one by one going forward(from ALU2 to Write back) to find the most recent instruction that has that source as its destination
  - When decode finds such an instruction, it marks the dependency from the decoded instuction to the producer
  - Decode (and ALU1 and ALU2 in the case of a store) checks if a source is valid, if it is not, it tries to find where the producer wrote to the fowarding structure, and retrieves that value
Stalls: 
  - When a stall is detected in the next instruction in the pipeline, we set stall[this stage] to be true, and write our own program counter to be the next instuction of this stage.
  
  Notes:
  The X register is represented in our simulator as AR 18