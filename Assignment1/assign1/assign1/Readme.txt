ADO Assignment1 : 

Submission

Group Members :
 
	Raghunath Reddy B 		(A20332674)
	
	Suryachandra Reddy Lankala	(A20321782)
	
	Alekhya Kancherla		(A20339622)



Acknowledgements: 
	
	We would like to  thank prof.Omar Aldawud and TA Ning Liu for the continuous guidance and support provided for the assignment to get completed with in the time bounds provided along with additional features too.



Assignment Implementations:
	

*This First Assignment includes implementation of Storage Manager which is capable of creating and opening a file, writing to specified location, 
reading from specified location and keeping track of current position using a custom data structure.
	

*Implemented createPageFile method for creating a file with initial page size 1 and size of 0B.

*Implemented openPageFile method for opening the created file and store the details of file using custom data structur SM_FileHanlde.
 
*Implemeted closePageFile and destroyPageFile methods for closing and deleting files.

		*Implemnted getBlockPos for keeping track of current position of the file.

*Read and write functionalities readCurrentBlock,readPreviousBlock,readNextBlock,readLastBlock,readFirstBlock,readBlock,writeBlock,writeCurrentBlock are implemented in such a way that any block in the file can be read and/or written. Each function implements definite architecture to keep track of ongoing events like changing the 
position parameter after writing the file. 
	

*Additional functionalities ensureCapacity for ensuring required number of pages are available and appendEmptyBlock to append empty blocks when required are implemented.


	*Implemented additional error codes returned such as RC_ENOMEM, RC_FILE_NAME_EMPTY, RC_FILE_BUSY which are returned depending on the errors occured with in our storage manager application.

*Test cases provided in the class test_assign1_1.c is executed successfully.

Compiling Instruction: 
We have implemented the following make files (makefile) 

1.makefile : This includes the test file test_assign1_1.c which has basic testing stategy.

	

Command to compile : make -f makefile
	
Output file generated: assign1
	Command to Run : ./assign1

