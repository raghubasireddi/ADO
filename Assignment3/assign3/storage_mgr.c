// START SECTION : Header files
#include<errno.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>
#include "dberror.h"
#include "storage_mgr.h"
// END SECTION: Header files


/*
  IMPLEMENTED BY: Raghu  MODULE: Initiating Storage Manager
  INPUT GIVEN: void  OUTPUT: void;
  UNIT TESTED BY: Raghu
*/

void initStorageManager()
{

printf("Welcome to the Storage Manager!! Initiated the storage Manager.");

}


/*
 IMPLEMENTED BY: Raghu /* MODULE: Creating a NEW page file
 INPUT GIVEN: fileName /* OUTPUT: RC_OK;RC_FILE_NOT_FOUND;
 UNIT TESTED BY: Raghu
*/


RC createPageFile(char *fileName)
{
	if(fileName == "" || fileName == NULL)
		return	RC_FILE_NAME_EMPTY;
	FILE *filePointer = fopen(fileName, "wb");
	char *temp = malloc(PAGE_SIZE);
        memset(temp, 0, PAGE_SIZE);
	fseek(filePointer, 0L, SEEK_SET);
        fwrite(temp,PAGE_SIZE,1, filePointer);
	if(filePointer == NULL)
		return RC_FILE_NOT_CREATED;
	free(temp);
	fclose(filePointer);
	return RC_OK;	

}

/*
  IMPLEMENTED BY: Alekya/* MODULE:Opening the page file created
  INPUT GIVEN: fileName,file handler object /* OUTPUT: RC_OK;RC_FILE_NOT_FOUND;
  UNIT TESTED BY: Alekya
*/


RC openPageFile(char *fileName, SM_FileHandle *fHandle)
{
	FILE *filePointer = NULL;
	fHandle->mgmtInfo = (void*) malloc(PAGE_SIZE);
	if(fHandle == NULL)
		return RC_FILE_HANDLE_EMPTY;
		
	if(fHandle->mgmtInfo == NULL)
		return RC_ENOMEM;
	struct stat tempBuffer;
	if(stat(fileName, &tempBuffer) != 0)
	{
		if(errno == EOVERFLOW)
			return RC_RM_UNKOWN_DATATYPE;
		if(errno == ENOENT)
			return RC_FILE_NOT_FOUND;
		if(errno == EOVERFLOW)
			return RC_RM_UNKOWN_DATATYPE;
		return RC_FILE_NOT_FOUND;
	}
	int fileDescriptor = open(fileName, O_RDWR);
        filePointer = fdopen(fileDescriptor,"wb"); 
	
	if(filePointer == NULL)
	{
		return RC_FILE_NOT_FOUND;
	}
	else
	{
		fHandle->fileName = fileName;
		fHandle->totalNumPages = 1;
		fHandle->curPagePos = 0;
		fHandle->mgmtInfo = filePointer;
		fseek(filePointer, 0L, SEEK_SET);
		fwrite(fHandle, PAGE_SIZE, 1, filePointer);
		return RC_OK;
	}
}

/*
  IMPLEMENTED BY: Surya/* MODULE:closing the page file 
  INPUT GIVEN: file handler object /
  OUTPUT: RC_FILE_HANDLE_EMPTY;RC_FILE_NOT_OPENED;
    RC_FILE_NOT_CLOSED, RC_OK
  UNIT TESTED BY: Surya
*/


RC closePageFile(SM_FileHandle *fHandle)
{
	
	if(fHandle == NULL)
		return RC_FILE_HANDLE_EMPTY;
	if(fHandle->mgmtInfo == NULL)
		return RC_FILE_NOT_OPENED;
	FILE *filePointer = fHandle->mgmtInfo;
	if(fclose(filePointer)!= 0)
		return RC_FILE_NOT_CLOSED;
	else
	{

		fHandle->mgmtInfo = NULL;
		filePointer = 0;
	}
	return RC_OK;
	
}

/*
  IMPLEMENTED BY: Alekya/* MODULE:destroying the page file 
  INPUT GIVEN: fileName /
  OUTPUT:RC_FILE_NOT_FOUND;RC_RM_UNKOWN_DATATYPE
    RC_FILE_BUSY;RC_FILE_NOT_DELETED;RC_OK
  UNIT TESTED BY: Surya
*/

RC destroyPageFile(char *fileName)
{
	struct stat fileTemp;
	if(stat(fileName, &fileTemp) != 0)
	{
		if(errno == ENOENT)
			return RC_FILE_NOT_FOUND;
		if(errno == EOVERFLOW)
			return RC_RM_UNKOWN_DATATYPE;
	}
	if(unlink(fileName) != 0)
	{
		if(errno == EBUSY)
			return RC_FILE_BUSY;
		return RC_FILE_NOT_DELETED;
	}
	return RC_OK;
}

/*
  IMPLEMENTED BY: Alekya/* MODULE:reading the block 
  INPUT GIVEN: pageNum as integer,FileHandler as object,pageHandle/
  OUTPUT:RC_FILE_HANDLE_EMPTY;RC_FILE_NOT_OPENED;
    RC_OK; blockPos
  UNIT TESTED BY: Alekya
*/

RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
{


	FILE *filePointer = NULL;
	if(fHandle == NULL)
		return RC_FILE_HANDLE_EMPTY;
	if(fHandle->mgmtInfo == NULL)
		return RC_FILE_NOT_OPENED;
	filePointer = fHandle->mgmtInfo;
	if(fseek(filePointer, (pageNum + 1) * PAGE_SIZE, SEEK_SET)< 0)
		return 555;
	if(fread(memPage, PAGE_SIZE, 1, filePointer) == -1)
		return 666;
	fHandle->curPagePos = (ftell(filePointer)/PAGE_SIZE) - 1;
	return RC_OK;		
}

/*
  IMPLEMENTED BY: Raghu/* MODULE:returns the current block position 
  INPUT GIVEN: FileHandler as object/
  OUTPUT:Block Position as Integer; blockPos
  UNIT TESTED BY: Raghu
*/

int getBlockPos (SM_FileHandle *fHandle)
{
	int blockPos;
	FILE *filePointer = NULL;
	if(fHandle == NULL)
		return RC_FILE_HANDLE_EMPTY;
	if(fHandle->mgmtInfo == NULL)
		return RC_FILE_NOT_OPENED;
	filePointer = fHandle->mgmtInfo;
	blockPos = ftell(filePointer);
	return blockPos;
}

/*
  IMPLEMENTED BY: Raghu/* MODULE:reading the first block 
  INPUT GIVEN: FileHandler as object,Pagehandle   
  OUTPUT:RC_FILE_HANDLE_EMPTY;RC_FILE_NOT_OPENED;RC_OK; 
  UNIT TESTED BY: Raghu
*/

RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	FILE *filePointer = NULL;
	if(fHandle == NULL)
		return RC_FILE_HANDLE_EMPTY;
	if(fHandle->mgmtInfo == NULL)
		return RC_FILE_NOT_OPENED;
	filePointer = fHandle->mgmtInfo;
	fseek(filePointer, 0L, SEEK_SET);
	fread(memPage, PAGE_SIZE, 1, filePointer);
	fHandle->curPagePos = (ftell(filePointer)/PAGE_SIZE) - 1;
	return RC_OK;		
}

/*
  IMPLEMENTED BY: Raghu/* MODULE:reading the previous block 
  INPUT GIVEN: FileHandler as object,Pagehandle   
  OUTPUT:RC_FILE_HANDLE_EMPTY;RC_FILE_NOT_OPENED;RC_OK; 
  UNIT TESTED BY: Raghu
*/


RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	FILE *filePointer = NULL;
	if(fHandle == NULL)
		return RC_FILE_HANDLE_EMPTY;
	if(fHandle->mgmtInfo == NULL)
		return RC_FILE_NOT_OPENED;
	filePointer = fHandle->mgmtInfo;
	fseek(filePointer, fHandle->curPagePos * PAGE_SIZE, SEEK_CUR);
	fread(memPage, PAGE_SIZE, 1, filePointer);
	fHandle->curPagePos = (ftell(filePointer)/PAGE_SIZE) - 1;
	return RC_OK;	
}


/*
  IMPLEMENTED BY:Surya /* MODULE:reading the currentblock 
  INPUT GIVEN: FileHandler as object,Pagehandle   
  OUTPUT:RC_FILE_HANDLE_EMPTY;RC_FILE_NOT_OPENED;RC_OK; 
  UNIT TESTED BY: Surya
*/

RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	FILE *filePointer = NULL;
	if(fHandle == NULL)
		return RC_FILE_HANDLE_EMPTY;
	if(fHandle->mgmtInfo == NULL)
		return RC_FILE_NOT_OPENED;
	filePointer = fHandle->mgmtInfo;
	fseek(filePointer, (fHandle->curPagePos + 1) * PAGE_SIZE, SEEK_CUR);
	fread(memPage, PAGE_SIZE, 1, filePointer);
	fHandle->curPagePos = (ftell(filePointer)/PAGE_SIZE) - 1;
	return RC_OK;	
}

/*
  IMPLEMENTED BY:Alekya /* MODULE:reading the next block 
  INPUT GIVEN: FileHandler as object,Pagehandle   
  OUTPUT:RC_FILE_HANDLE_EMPTY;RC_FILE_NOT_OPENED;RC_OK; 
  UNIT TESTED BY: Alekya
*/


RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	FILE *filePointer = NULL;
	if(fHandle == NULL)
		return RC_FILE_HANDLE_EMPTY;
	if(fHandle->mgmtInfo == NULL)
		return RC_FILE_NOT_OPENED;
	filePointer = fHandle->mgmtInfo;
	fseek(filePointer, (fHandle->curPagePos + 2) * PAGE_SIZE, SEEK_CUR);
	fread(memPage, PAGE_SIZE, 1, filePointer);
	fHandle->curPagePos = (ftell(filePointer)/PAGE_SIZE) - 1;
	return RC_OK;
}
/*
  IMPLEMENTED BY:Surya /* MODULE:reading the last block 
  INPUT GIVEN: FileHandler as object,Pagehandle   
  OUTPUT:RC_FILE_HANDLE_EMPTY;RC_FILE_NOT_OPENED;RC_OK; 
  UNIT TESTED BY: Surya
*/

RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	FILE *filePointer = NULL;
	if(fHandle == NULL)
		return RC_FILE_HANDLE_EMPTY;
	if(fHandle->mgmtInfo == NULL)
		return RC_FILE_NOT_OPENED;
	filePointer = fHandle->mgmtInfo;
	fseek(filePointer, PAGE_SIZE, SEEK_END);
	fread(memPage, PAGE_SIZE, 1, filePointer);
	fHandle->curPagePos = (ftell(filePointer)/PAGE_SIZE) - 1;
	return RC_OK;
}

/*
  IMPLEMENTED BY:Raghu /* MODULE:writing the block 
  INPUT GIVEN: FileHandler as object,Pagehandle   
  OUTPUT:RC_FILE_HANDLE_EMPTY;RC_FILE_NOT_OPENED;RC_OK; 
  UNIT TESTED BY: Raghu
*/

RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
{

	FILE *filePointer = NULL;
	if(fHandle == NULL)
		return RC_FILE_HANDLE_EMPTY;
	if(fHandle->mgmtInfo == NULL)
		return RC_FILE_NOT_OPENED;
	filePointer = fHandle->mgmtInfo;
	fseek(filePointer, (pageNum + 1) * PAGE_SIZE, SEEK_SET);
	fwrite(memPage, PAGE_SIZE, 1, filePointer);
	fHandle->curPagePos = (ftell(filePointer)/PAGE_SIZE) - 1;
	fHandle->totalNumPages = fHandle->totalNumPages + 1;
	return RC_OK;
}


/*
  IMPLEMENTED BY:Alekya /* MODULE:writing the current block 
  INPUT GIVEN: FileHandler as object,Pagehandle   
  OUTPUT:RC_FILE_HANDLE_EMPTY;RC_FILE_NOT_OPENED;RC_OK; 
  UNIT TESTED BY: Alekya
*/

RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	FILE *filePointer = NULL;
	if(fHandle == NULL)
		return RC_FILE_HANDLE_EMPTY;
	if(fHandle->mgmtInfo == NULL)
		return RC_FILE_NOT_OPENED;
	filePointer = fHandle->mgmtInfo;
	fseek(filePointer, (fHandle->curPagePos + 1) * PAGE_SIZE, SEEK_SET);
	fwrite(memPage, PAGE_SIZE, 1, filePointer);
	fHandle->curPagePos = (ftell(filePointer)/PAGE_SIZE) - 1;
	fHandle->totalNumPages = fHandle->totalNumPages + 1;
	return RC_OK;
}

/*
  IMPLEMENTED BY:Raghu /* MODULE:appending the empty block 
  INPUT GIVEN: FileHandler as object,Pagehandle   
  OUTPUT:RC_FILE_HANDLE_EMPTY;RC_FILE_NOT_OPENED;RC_OK; 
  UNIT TESTED BY: Alekya
*/

RC appendEmptyBlock (SM_FileHandle *fHandle)
{
	char *emptyCharacter = (void*)malloc(sizeof(PAGE_SIZE));
	FILE *filePointer = NULL;
	if(fHandle == NULL)
		return RC_FILE_HANDLE_EMPTY;
	if(fHandle->mgmtInfo == NULL)
		return RC_FILE_NOT_OPENED;
	filePointer = fHandle->mgmtInfo;
	fseek(filePointer, 0L, SEEK_END);
	fwrite(emptyCharacter, PAGE_SIZE, 1, filePointer);
	fHandle->curPagePos = (ftell(filePointer)/PAGE_SIZE) - 1;
	fHandle->totalNumPages = fHandle->totalNumPages + 1;
	return RC_OK;
}

/*
  IMPLEMENTED BY:Alekya /*MODULE:ensuring the capacity of       pagefile 
  INPUT GIVEN: numberOfPages as integer,FileHandler as object   
  OUTPUT:RC_FILE_HANDLE_EMPTY;RC_FILE_NOT_OPENED;RC_OK; 
  UNIT TESTED BY: Surya
*/


RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle)
{
	int i;
	char *emptyCharacter = (void*)malloc(sizeof(PAGE_SIZE));
	FILE *filePointer = NULL;
	if(fHandle == NULL)
		return RC_FILE_HANDLE_EMPTY;
	if(fHandle->mgmtInfo == NULL)
		return RC_FILE_NOT_OPENED;
	filePointer = fHandle->mgmtInfo;
	if(fHandle->totalNumPages < numberOfPages)
	{
		for(i = fHandle->totalNumPages; i< numberOfPages; i++)
		{
			fwrite(emptyCharacter, PAGE_SIZE, 1, filePointer);
			fHandle->totalNumPages = fHandle->totalNumPages + 1;
		} 			
	}
	fHandle->curPagePos = (ftell(filePointer)/PAGE_SIZE) - 1;
	return RC_OK;	
}
