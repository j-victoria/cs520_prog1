# cs520_prog1
Simulator for the simple APEX pipeline discussed in class

To-Do:

1. Establish how we are actually going to do the project
2. ???
3. Profit

But actually (not necessarily in this order): 

1. Figure out the order in which the project is going to be done. (I was thinking ALU instrutions w/ no forwarding, then add forwarding, then add branch instructions?)
2. Determine how we are going to represent intructions, registers, memory, stages in the pipeline
  i.e. is each stage in the pipeline a method of an instruction class? or is each stage its own class that has an instruction pointer as one of its field?
3. file parsing/ string parsing (should be fairly easy) 
4. Impliment the master-slave latch in program. (barriers?)
5. Forwarding (determined by 2, aka, how to communicate between methods/classes)
6. Branches/squashing 
