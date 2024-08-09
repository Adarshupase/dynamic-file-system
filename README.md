# dynamic-file-system-simulation
Dynamic file system is a file system simulation in which the block groups are flexible which prevents internal fragmentation


The main idea behind this file system model was the overhead of allocating the block pointers for large files 
in this model the large files are allocated a group of that size preventing us from maintaing pointers to differnet groups.


At the start each group contain 64 blocks as the first block gets written the group changes by changing its pointers for both the (group_start) and (group_end) block 
updating the file is rather different. to take care of the external fragmentation which may happen in such situation a group is chosen on the basis of first fit algorithm and then the next_group pointer is updated.


most of the file system properties are similar to taditional filesystems like FAT and ext 
this model is still in its beginning phase you can't just mount it now 


here is the overview 
+----------+----------+----------+----------+ .... +----------+
|  G1(64)  |  G2(64)  |  G3(64)  |  G4(64)  | .... | G64(64)  |
+----------+----------+----------+----------+ .... +----------+

all groups are of size 64 if i write a file which occupies 34 blocks then the file will just occupy the 
first 34 blocks and then the group structure looks something like this
+----------+------------+----------+----------+ .... +----------+
|  G1(34)  |  G2(94)    |  G3(64)  |  G4(64)  | .... | G64(64)  |
+----------+------------+----------+----------+ .... +----------+
imagine if the final structure after allocating some files look like

+----------+----------+----------+----------+ .... +----------+
|  G1(34)  |  G2(60)  |  G3(64)  |  G4(33)  | .... | G64(64)  |
+----------+----------+----------+----------+ .... +----------+
this looks like G1 G2 G3 G4 are already occupied inorder to update the file residing in G1 
we allocate a group pointer which points to the first block of next group which holds the data 

