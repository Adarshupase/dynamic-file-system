# dynamic-file-system-simulation
Dynamic file system is a file system simulation in which the block groups are flexible which prevents internal fragmentation
The mainm idea behind this file system model was the overhead of allocating the block pointers for large files 
in this model the large files are allocated a group of that size preventing us from maintaing pointers to the group 
at the start each group contain 64 blocks as the first block gets written the group changes by changing its pointers for bothe the (group_start) and (group_end) block 
updating the file is rather different to take care of the external fragmentation which may arrive in such situation a group is chosen on the basis of first fit algorithm 
most of the file system properties are similar to taditional filesystems like FAT and ext 
this model is still in its beginning phase you can't just mount it now 
