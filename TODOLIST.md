# TO DO LIST

## Overall
Marvin: Documentation, refactoring

Jay: Coding etc.

## Parts
Reading in ASCII input file: Needs to be changed to read filename from command line instead of user prompt

Also: Handling pipeline when the last instruction in a file is in the pipeline (avoiding out of bounds instruction access)

fetch(): mostly good

decode(): BZ/BNZ zero flag dependencies (+ register-with-most-recently-set-zero-flag pointer register), X register dependencies, dependencies all around!!!!!!

alu1(): 

alu2():

beu(): calculating branch target address

delay(): NOP squashing

mem():

wb():

main(): adjust Clearing out to handle all globals
        add in 'end of processing' handleing'
        add in error processing
        command processing???

general debugging.