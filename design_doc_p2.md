
 - RAT & RRAT are arrays or integers which are the index in the urf of the register it points to
 - IQ is an array of structucts where one member is a pointer to an instruction struct which represents the instruction that IQE is holding
 - ROB is an array with head and tail pointers 
 - the URF is an array of register structures, each includes a list of instructions waiting on it's value
 - FU's linearly probe the IQ for an instruction that meets the critea and runs it
 - 
