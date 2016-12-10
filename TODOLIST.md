 - Set_URF_size < n>: used before simulation to set the number of registers in the unified register file
to n.
 - Print_map_tables: prints front rename table and back-end register alias table.
 - Print_ROB: prints current ROB contents, one entry per line.
 - Print_URF: Prints contents of URF and their status (allocated, committed, free)
 - Print_Memory <a1> <a2>: prints out contents of memory locations with addresses ranging from
a1 to a2, both inclusive. The addresses a1 and a2 are at 4 Byte boundaries.
 - Print_Stats: prints the IPC realized up to the point where this command is invoked, the number of
cycles for which dispatched has stalled, the number of cycles for which no issues have taken place
to any function unit, number of LOAD and STORE instructions committed (separately).

- ~Fix ptr problem ~ (thanks marvin)
- test iq
- test ROB 
- debuggin!
- make simulate stop on halt
- take more input instructions ( like multiple simulates)
- fix init :(
