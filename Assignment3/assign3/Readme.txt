ADO Assignment2 : Submission

Group Members : 
	Raghunath Reddy B 		(A20332674)
	Suryachandra Reddy Lankala	(A20321782)
	Alekhya Kancherla		(A20339622)

Acknowledgements: 
	We would like to  thank prof.Omar Aldawud and TA Ning Liu for the continuous guidance and support provided for the assignment phase2 to get completed with in the time bounds provided along with additional features too.

Assignment Implementations:

*This Second Assignment includes implementation of Buffer Manager which is capable of managing the buffer pool of given pageframes and specified size, with different page replacement algorithms using custom data structure.

*Implemented Queue Buffer Pool Page replacement algorithm : First In First Out Implementation

*Implemented LRU Buffer Pool Page replacement algorithm : Least Recently Used Pages to be evicted.
 
*Implemeted Clock Buffer Pool Page replacement algorith: Clock Handle evicts pages with R value 0.

The following methods are been implemented for the Buffer pool Purpose: 
 initBufferPool, shutdownBufferPool, forceFlushPool
 markDirty, unpinPage, forcePage, pinPage
 getFrameContents, getDirtyFlags, getFixCounts, getNumReadIO, getNumWriteIO



*Implemented additional error codes returned such as RC_BUFFER_EXCEEDED which are returned depending on the errors occured with in our Buffer manager application.

*Test cases provided in the class test_assign2_1.c is executed successfully.

*Test cases provided in the class test_assign2_2.c is executed successfully.

Compiling Instruction: 
We have implemented the following make files (makefile) 

1.Makefile : This includes the test file test_assign2_1.c which has basic testing strategy for FIFO, LRU .

	Command to compile : make -f Makefile
	Output file generated: assign2
	Command to Run : ./assign2

Execute Clean of Makefile1:
	Please execute the clean section of Makefile1 by following command: 
	Command: make -f Makefile clean

2.Makefile2 : This includes the test file test_assign2_2.c which has additional testing strategy for CLOCK,  FIFO, LRU for Random based values

	Command to compile : make -f Makefile2
	Output file generated: assign2_2
	Command to Run : ./assign2_2


