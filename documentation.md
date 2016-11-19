Stage:
  - each stage is implimented as a function
  - every stage/function takes an integer which is the index of instruction to be processed in that stage
  - every stage returns an integer refering to whether an error was found in that stage
  - each stage writes to a strucuture the index of the instruction that will be processed in the next stage
Forward:
  - ALU2, Branch FU, Delay, Memory and Wriback write their destination, result and their instruction's program counter to a fowarding structure
  - Decode will detect a source argument, try and find it in the register file, and, if it cannot, checks stages one by one going forward(from ALU2 to Write back) to find the most recent instruction that has that source as its destination
  - When decode finds such an instruction, it marks the dependency from the decoded instuction to the producer
  - Decode (and ALU1 and ALU2 in the case of a store) checks if a source is valid, if it is not, it tries to find where the producer wrote to the fowarding structure, and retrieves that value
Stalls: 
  - When a stall is detected in the next instruction in the pipeline, we set stall[this stage] to be true, and write our own program counter to be the next instuction of this stage.