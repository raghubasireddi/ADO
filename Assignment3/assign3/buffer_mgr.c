/* Start Header Files*/
#include "storage_mgr.h"
#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include <time.h>
#include "buffer_mgr.h"
#include "buffer_mgr_stat.h"
#include "dberror.h"
#include "dt.h"
/* End Header Files*/

/* Enum to define the Methodologty of Current Page Frame*/
typedef enum FrameMethodology {
    MARK_Pin = 0,
    MARK_Dirty = 1,
    MARK_Unpin = 2,
    MARK_NOT_Dirty = 3
} FrameMethodology;

/* Defining Strategy Buffer*/
typedef struct Buffer_Strategy
{
	SM_FileHandle *fileHandle;
	void *strategyMemory;
	int isDataRead;
	int isDataWrite;
	int *fixedCountVariable;
	bool *dirtyPagesVariable;
	PageNumber *pageNumberVariable;
	
} Buffer_Strategy;
/* Defining the Frontier to implement Page Replacement Algorithm First In First Out*/
struct Frontier_Queue
{
    int position;
    int pageNumber;
    bool isDirty;
    int fixCount;
    char *pageData;
    struct Frontier_Queue *nextNode;
} *first_Fifo = NULL, *last_Fifo = NULL, *temp_Fifo = NULL, *handler=NULL;
/* Defining the Frontier to implement Page Replacement Algorithm Least Recently Used*/		 
struct Frontier_LRU {
    
    int fixCount;
    bool isDirty;
    int timeStamp;
    char *pageData;
    int position;
    int pageNumber;
    struct Frontier_LRU *nextNode;
} *lru_Frontier = NULL;

/* Defining Frontier to implement Page Replacement Algorithm Clock*/
struct Frontier_Clock {
    int position;
    int pageNumber;
    int fixCount;
    bool isDirty;
    char *pageData;
    int reference;
    struct Frontier_Clock *nextNode;
} *first_Clock = NULL, *tickle_Clock = NULL;

#define MAKE_FILE_HANDLE()				\
  ((SM_FileHandle *) malloc(PAGE_SIZE))

/*
  IMPLEMENTED BY: Raghu MODULE:Initializing BufferPool
  INPUT GIVEN: bm,pageFileName,numPages,ReplacementStrategy,stratData 
  OUTPUT: RC_BUFFER_STRATEGY_UNSPECIFIED;RC_OK; 
  UNIT TESTED BY: Alekya
*/

RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, 
		  const int numPages, ReplacementStrategy strategy, 
		  void *stratData){
	int j = 0;

	Buffer_Strategy *buffer_Strategy = ((Buffer_Strategy *) malloc (sizeof(Buffer_Strategy)));

	bm->pageFile = (char *) pageFileName;
   	bm->numPages = numPages;
    bm->strategy = strategy;

	SM_FileHandle *fileHandle = MAKE_FILE_HANDLE();
	memset(fileHandle,0,PAGE_SIZE);

   	buffer_Strategy->pageNumberVariable = (PageNumber *) malloc(sizeof (PageNumber) * numPages);
    	buffer_Strategy->fixedCountVariable = (int *) malloc(sizeof (int)*numPages);
    	buffer_Strategy->dirtyPagesVariable = (bool *) malloc(sizeof (bool) * numPages);

    	for (j = 0; j < numPages; j++) {
     	   	buffer_Strategy->pageNumberVariable[j] = NO_PAGE;
        	buffer_Strategy->fixedCountVariable[j] = 0;
        	buffer_Strategy->dirtyPagesVariable[j] = false;
    	}
    	buffer_Strategy->isDataRead = 0;
	buffer_Strategy->isDataWrite = 0;

	openPageFile((char *) pageFileName, fileHandle);
	buffer_Strategy->fileHandle = fileHandle;
	if(strategy == RS_FIFO) {

	   	buffer_Strategy->strategyMemory = first_Fifo;
	    
	}
	else if(strategy == RS_LRU){

		buffer_Strategy->strategyMemory = lru_Frontier;
            
	}
	else if(strategy == RS_CLOCK){

		buffer_Strategy->strategyMemory = first_Clock;
            
	}
	else{
	return RC_BUFFER_STRATEGY_UNSPECIFIED;
	}
	bm->mgmtData = buffer_Strategy;	
	return RC_OK;
}

/*
  IMPLEMENTED BY: Alekya MODULE: Shuttingdown BufferPool
  INPUT GIVEN: bm 
  OUTPUT: RC_BUFFER_STRATEGY_UNSPECIFIED;RC_OK; 
  UNIT TESTED BY: Surya
*/
RC shutdownBufferPool(BM_BufferPool *const bm){
	struct Frontier_Queue *frontier_Queue = ((struct Frontier_Queue *) malloc (sizeof(struct Frontier_Queue)));
	struct Frontier_LRU *frontier_LRU = ((struct Frontier_LRU *) malloc (sizeof(struct Frontier_LRU)));
	struct Frontier_Clock *frontier_Clock = ((struct Frontier_Clock *) malloc (sizeof(struct Frontier_Clock)));
	struct Buffer_Strategy *bufferStrategy = ((struct Buffer_Strategy *) malloc (sizeof(struct Buffer_Strategy)));
	bufferStrategy = bm->mgmtData;
	
	if(bm->strategy == RS_FIFO)
	{
		frontier_Queue = first_Fifo;
		while(frontier_Queue != NULL)
			{
				if(frontier_Queue->isDirty)
				{
					writeBlock(frontier_Queue->pageNumber, bufferStrategy->fileHandle, frontier_Queue->pageData);
                    			bufferStrategy->isDataWrite = bufferStrategy->isDataWrite + 1;
				}
				frontier_Queue = frontier_Queue->nextNode;
			}
		
	}
	else if(bm->strategy == RS_LRU){
		frontier_LRU = lru_Frontier;
		while (frontier_LRU != NULL) {
	                if (frontier_LRU->isDirty) {
				writeBlock(frontier_LRU->pageNumber, bufferStrategy->fileHandle, frontier_LRU->pageData);
				bufferStrategy->isDataWrite = bufferStrategy->isDataWrite + 1;
	                }
	                frontier_LRU = frontier_LRU->nextNode;
		}

	}
	else if(bm->strategy == RS_CLOCK){
		frontier_Clock = first_Clock;
		if (frontier_Clock->nextNode == first_Clock) {
			writeBlock(frontier_Clock->pageNumber, bufferStrategy->fileHandle, frontier_Clock->pageData);
			bufferStrategy->isDataWrite = bufferStrategy->isDataWrite + 1;
		} else {
				do {  
					if (frontier_Clock->isDirty) {
						writeBlock(frontier_Clock->pageNumber, bufferStrategy->fileHandle, frontier_Clock->pageData);
						bufferStrategy->isDataWrite = bufferStrategy->isDataWrite + 1;
					}
					frontier_Clock = frontier_Clock->nextNode;
				} while(frontier_Clock->nextNode != first_Clock);
		}
	}
	else{
		return RC_BUFFER_STRATEGY_UNSPECIFIED;
	}
	first_Fifo = NULL;
	last_Fifo = NULL;
	handler = NULL;
	lru_Frontier = NULL;
	closePageFile(bufferStrategy->fileHandle);
	free(frontier_Queue);
 	free(frontier_LRU);
 	free(frontier_Clock);
	free(bufferStrategy->dirtyPagesVariable);
	free(bufferStrategy->fixedCountVariable);
	free(bufferStrategy->pageNumberVariable);
	free(bufferStrategy->fileHandle);
	free(bufferStrategy->strategyMemory);
	
	free(bm->mgmtData);
	return RC_OK;
}
/*
  IMPLEMENTED BY: Surya MODULE: Forcely Flushing the Pool
  INPUT GIVEN: bm 
  OUTPUT: RC_BUFFER_STRATEGY_UNSPECIFIED;RC_OK; 
  UNIT TESTED BY: Raghu
*/
RC forceFlushPool(BM_BufferPool *const bm){

	struct Frontier_Queue *frontier_Queue = ((struct Frontier_Queue *) malloc (sizeof(struct Frontier_Queue)));
	struct Frontier_LRU *frontier_LRU = ((struct Frontier_LRU *) malloc (sizeof(struct Frontier_LRU)));
	struct Frontier_Clock *frontier_Clock = ((struct Frontier_Clock *) malloc (sizeof(struct Frontier_Clock)));
	struct Buffer_Strategy *bufferStrategy = ((struct Buffer_Strategy *) malloc (sizeof(struct Buffer_Strategy)));
	bufferStrategy = bm->mgmtData;
	
	if(bm->strategy == RS_FIFO)
	{
		frontier_Queue = first_Fifo;
		while(frontier_Queue != NULL){
			if(frontier_Queue->isDirty)
			{
				writeBlock(frontier_Queue->pageNumber,bufferStrategy->fileHandle, frontier_Queue->pageData);
		                bufferStrategy->isDataWrite = bufferStrategy->isDataWrite + 1;
				frontier_Queue->isDirty = false;
				bufferStrategy->dirtyPagesVariable[frontier_Queue->position] = frontier_Queue->isDirty;
	
			}
			frontier_Queue = frontier_Queue->nextNode;
		}
	}
	else if(bm->strategy == RS_LRU)
	{	
		frontier_LRU = lru_Frontier;
		while (frontier_LRU != NULL) {
			if (frontier_LRU->isDirty) {
				writeBlock(frontier_LRU->pageNumber, bufferStrategy->fileHandle, frontier_LRU->pageData);
				bufferStrategy->isDataWrite = bufferStrategy->isDataWrite + 1;
			}
			frontier_LRU = frontier_LRU->nextNode;
		}
	}
	else if(bm->strategy == RS_CLOCK)
	{
		frontier_Clock = first_Clock;
		if (frontier_Clock->nextNode == first_Clock) {
			writeBlock(frontier_Clock->pageNumber, bufferStrategy->fileHandle, frontier_LRU->pageData);
			bufferStrategy->isDataWrite = bufferStrategy->isDataWrite + 1;
		} 
		else {
			do{
				if (frontier_Clock->isDirty) {
					writeBlock(frontier_Clock->pageNumber, bufferStrategy->fileHandle, frontier_LRU->pageData);
					bufferStrategy->isDataWrite = bufferStrategy->isDataWrite + 1;
				}
				frontier_Clock = frontier_Clock->nextNode;
			}while (frontier_Clock->nextNode != first_Clock);
		}
	}

	else{
		return RC_BUFFER_STRATEGY_UNSPECIFIED;
	}
 	free(frontier_Queue);
 	free(frontier_LRU);
	return RC_OK;
}

/*
  IMPLEMENTED BY: Raghu MODULE: Mark a page as Dirty
  INPUT GIVEN: bm,page
  OUTPUT: RC_BUFFER_STRATEGY_UNSPECIFIED;RC_OK; 
  UNIT TESTED BY: Alekya
*/
RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page){
	struct Frontier_Queue *frontier_Queue = ((struct Frontier_Queue *) malloc (sizeof(struct Frontier_Queue)));
	struct Frontier_LRU *frontier_LRU = ((struct Frontier_LRU *) malloc (sizeof(struct Frontier_LRU)));
	struct Buffer_Strategy *bufferStrategy = ((struct Buffer_Strategy *) malloc (sizeof(struct Buffer_Strategy)));
	struct Frontier_Clock *frontier_Clock = ((struct Frontier_Clock *) malloc (sizeof(struct Frontier_Clock)));
	bufferStrategy = bm->mgmtData;
	int framePosition = -1;
	
	int timeStamp_LRU;
	if(bm->strategy == RS_FIFO)
	{
		frontier_Queue = first_Fifo;
		framePosition = search_QUEUE_Frontier(page->pageNum);
		if(framePosition >= 0)
		{
			while(frontier_Queue != NULL)
			{
				if((frontier_Queue->position) == framePosition)
				{
					frontier_Queue->pageData = page->data;
					update_QUEUE_Frontier(frontier_Queue, bm->mgmtData, MARK_Dirty, framePosition);
				}
				frontier_Queue = frontier_Queue->nextNode;
			}
		}
	}
	else if(bm->strategy == RS_LRU)
	{
		frontier_LRU = lru_Frontier;

		timeStamp_LRU = frontier_LRU->timeStamp; 
		framePosition = frontier_LRU->position;
		while (frontier_LRU != NULL) {
			if (frontier_LRU->timeStamp > timeStamp_LRU) {
				timeStamp_LRU = frontier_LRU->timeStamp;
				framePosition = frontier_LRU->position;
			}
			frontier_LRU = frontier_LRU->nextNode;
		}
    
		frontier_LRU = lru_Frontier;
		while (frontier_LRU != NULL) {
			if (frontier_LRU->position == framePosition) {
				frontier_LRU->isDirty = true;
				bufferStrategy->dirtyPagesVariable[frontier_LRU->position] = frontier_LRU->isDirty;
			}
			frontier_LRU = frontier_LRU->nextNode;
		}

	}
	else if(bm->strategy == RS_CLOCK)
	{
		framePosition = getFrameClockPosition(page->pageNum);
		frontier_Clock = first_Clock;
		if (framePosition != -1) {
			if (framePosition == first_Clock->position) {
		
				first_Clock->pageData=page->data;
				first_Clock->isDirty = true;
				bufferStrategy->dirtyPagesVariable[first_Clock->position] = first_Clock->isDirty;
			} 
			else {
				do{
					if (frontier_Clock->position == framePosition) {
						frontier_Clock->pageData=page->data;
						frontier_Clock->isDirty = true;
						bufferStrategy->dirtyPagesVariable[frontier_Clock->position] = frontier_Clock->isDirty;
					}
					frontier_Clock = frontier_Clock->nextNode;
				}while (frontier_Clock->nextNode != first_Clock);
			}
		}
	}
	else{
		return RC_BUFFER_STRATEGY_UNSPECIFIED;
	}
	return RC_OK;
}

/*
  IMPLEMENTED BY: Alekhya MODULE: unpinPage
  INPUT GIVEN: bm,page
  OUTPUT: RC_BUFFER_STRATEGY_UNSPECIFIED;RC_OK; 
  UNIT TESTED BY: Alekya
*/
RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page){
	struct Frontier_Queue *frontier_Queue = ((struct Frontier_Queue *) malloc (sizeof(struct Frontier_Queue)));
	struct Frontier_LRU *frontier_LRU = ((struct Frontier_LRU *) malloc (sizeof(struct Frontier_LRU)));
	struct Frontier_Clock *frontier_Clock = ((struct Frontier_Clock *) malloc (sizeof(struct Frontier_Clock)));
	struct Buffer_Strategy *bufferStrategy = ((struct Buffer_Strategy *) malloc (sizeof(struct Buffer_Strategy)));
	bufferStrategy = bm->mgmtData;
	int framePosition = -1;
	int timeStamp_LRU;
	if(bm->strategy == RS_FIFO)
	{
		frontier_Queue = first_Fifo;
		framePosition = search_QUEUE_Frontier(page->pageNum);
		if(framePosition >= 0)
		{
			while(frontier_Queue != NULL)
			{
				if((frontier_Queue->position) == framePosition)
				{
					
					update_QUEUE_Frontier(frontier_Queue, bm->mgmtData, MARK_Unpin, framePosition);
				}
				frontier_Queue = frontier_Queue->nextNode;
			}
		}
	}
	else if(bm->strategy == RS_LRU)
	{
		
		framePosition = get_FramePosition_LRU(page->pageNum);
		frontier_LRU = lru_Frontier;
		while (frontier_LRU != NULL) {
			if (frontier_LRU->position == framePosition) {
			    frontier_LRU->fixCount = frontier_LRU->fixCount - 1;
			    bufferStrategy->fixedCountVariable[frontier_LRU->position] = frontier_LRU->fixCount;
			}
			frontier_LRU = frontier_LRU->nextNode;
		}
	}
	else if(bm->strategy == RS_CLOCK)
	{		
		framePosition = getFrameClockPosition(page->pageNum);
		frontier_Clock = first_Clock;
		if (framePosition != -1) {
			if (framePosition == first_Clock->position) {
				frontier_Clock->fixCount = frontier_Clock->fixCount - 1;
				bufferStrategy->fixedCountVariable[frontier_Clock->position] = frontier_Clock->fixCount;
			} 
			else {
				do{
					if (frontier_Clock->position == framePosition) {
						frontier_Clock->fixCount = frontier_Clock->fixCount - 1;
						bufferStrategy->fixedCountVariable[frontier_Clock->position] = frontier_Clock->fixCount;
					}
					frontier_Clock = frontier_Clock->nextNode;
				}while (frontier_Clock->nextNode != first_Clock);
			}
		}
	}
	else{
		return RC_BUFFER_STRATEGY_UNSPECIFIED;
	}
	return RC_OK;
}

/*
  IMPLEMENTED BY: Surya MODULE: unpinPage
  INPUT GIVEN: bm,page
  OUTPUT: RC_BUFFER_STRATEGY_UNSPECIFIED;RC_OK; 
  UNIT TESTED BY: Raghu
*/
RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page){
	int framePosition = -1;
	int timeStamp_LRU;
	struct Frontier_Queue *frontier_Queue = ((struct Frontier_Queue *) malloc (sizeof(struct Frontier_Queue)));
	struct Frontier_LRU *frontier_LRU = ((struct Frontier_LRU *) malloc (sizeof(struct Frontier_LRU)));
	struct Frontier_Clock *frontier_Clock = ((struct Frontier_Clock *) malloc (sizeof(struct Frontier_Clock)));	
	Buffer_Strategy *buffer_Strategy = ((Buffer_Strategy *) malloc (sizeof(Buffer_Strategy)));

	buffer_Strategy = bm->mgmtData;
	writeBlock(page->pageNum, buffer_Strategy->fileHandle, page->data);

	framePosition = search_QUEUE_Frontier(page->pageNum);
    	if(bm->strategy == RS_FIFO && framePosition != -1) {
        	while(frontier_Queue != NULL)
		{
               	    if (framePosition == frontier_Queue->position) {
			frontier_Queue->pageData=page->data;
			writeBlock(frontier_Queue->pageNumber, buffer_Strategy->fileHandle, page->data);
        	        update_QUEUE_Frontier(first_Fifo, buffer_Strategy, MARK_NOT_Dirty, framePosition);
			buffer_Strategy->isDataWrite++;
        	    } 
		    frontier_Queue = frontier_Queue->nextNode;
	        }
	}
	else if(bm->strategy == RS_LRU)
	{
		framePosition = get_FramePosition_LRU(page->pageNum);
		frontier_LRU = lru_Frontier;
		if (framePosition != -1) {
			while (frontier_LRU != NULL) {
				if (frontier_LRU->position == framePosition) {
					frontier_LRU->pageData=page->data;
					writeBlock(frontier_LRU->pageNumber, buffer_Strategy->fileHandle, page->data);
					frontier_LRU->isDirty = false;
					buffer_Strategy->dirtyPagesVariable[framePosition] = frontier_LRU->isDirty;
					buffer_Strategy->isDataWrite++;
				}
				frontier_LRU = frontier_LRU->nextNode;
			}
		}
	}
	else if(bm->strategy == RS_LRU)
	{
		framePosition = getFrameClockPosition(page->pageNum);
		frontier_Clock = first_Clock;
		if (framePosition != -1) {
			if (framePosition == first_Clock->position) {
				first_Clock->fixCount = first_Clock->fixCount - 1;
				buffer_Strategy->dirtyPagesVariable[framePosition] = first_Clock->isDirty;
			}
			else {

				do{
					if (frontier_Clock->position == framePosition) {
						frontier_Clock->pageData=page->data;
						writeBlock(frontier_Clock->pageNumber, buffer_Strategy->fileHandle, page->data);
						frontier_Clock->isDirty = false;
						buffer_Strategy->dirtyPagesVariable[framePosition] = frontier_Clock->isDirty;
						buffer_Strategy->isDataWrite++;
					}
					frontier_Clock = frontier_Clock->nextNode;
				}while (frontier_Clock->nextNode != first_Clock);
			}
		}

	}
	else{
		return RC_BUFFER_STRATEGY_UNSPECIFIED;
	}
	return RC_OK;
}

/*
  IMPLEMENTED BY: Raghu MODULE: pin a Page Frame when requested to
  INPUT GIVEN: bm,page
  OUTPUT: RC_BUFFER_STRATEGY_UNSPECIFIED;RC_OK; 
  UNIT TESTED BY: Alekya
*/
RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page, 
	    const PageNumber pageNum){
	int framePosition = -1;
	int timeStamp_LRU;	
	SM_PageHandle pageHandle = (SM_PageHandle) malloc(PAGE_SIZE);
	memset(pageHandle,0,PAGE_SIZE);
	Buffer_Strategy *buffer_Strategy = ((Buffer_Strategy *) malloc (sizeof(Buffer_Strategy)));
	struct Frontier_Clock *frontier_LRU = ((struct Frontier_Clock *) malloc (sizeof(struct Frontier_Clock)));
	
	buffer_Strategy = bm->mgmtData;
	page->pageNum = pageNum;
	if(bm->strategy == RS_FIFO){
		framePosition = search_QUEUE_Frontier(page->pageNum);
		readBlock(pageNum, buffer_Strategy->fileHandle, pageHandle);
		page->data=pageHandle;
		if (framePosition != -1) {
			update_QUEUE_Frontier(first_Fifo, buffer_Strategy, MARK_Pin, framePosition);
		} else {
			insert_Into_QUEUE_Frontier(bm, page);
		}
	}
	else if(bm->strategy == RS_LRU)
	{	
		if(get_FramePosition_LRU(page->pageNum)==-1){
			readBlock(pageNum, buffer_Strategy->fileHandle, pageHandle);
			buffer_Strategy->isDataRead = buffer_Strategy->isDataRead + 1;
			page->data=pageHandle;
		}
		
		insert_Into_Frontier_LRU(bm,page);
		
	}
	else if(bm->strategy == RS_CLOCK)
	{
		framePosition = getFrameClockPosition(page->pageNum);

		frontier_LRU = first_Clock;
		if (framePosition != -1) {
			if (framePosition == first_Clock->position) {
				first_Clock->fixCount++;
				buffer_Strategy->fixedCountVariable[first_Clock->position] = first_Clock->fixCount;
			} 
			else {
				do{
					if (frontier_LRU->position == framePosition) {
						frontier_LRU->fixCount++;
						buffer_Strategy->fixedCountVariable[first_Clock->position] = first_Clock->fixCount;
						buffer_Strategy->isDataRead = buffer_Strategy->isDataRead + 1;
						break;
					}
					frontier_LRU = frontier_LRU->nextNode;
				}while (frontier_LRU->nextNode != first_Clock);
			}
		}
		else {
			insert_Into_Clock_Buffer(bm, page);
			buffer_Strategy->isDataRead = buffer_Strategy->isDataRead + 1;
		}
	}
	else{
		return RC_BUFFER_STRATEGY_UNSPECIFIED;
	}
	return RC_OK;
}

/*
  IMPLEMENTED BY: Alekhya MODULE: Insert into a QUEUE Frontier when a new page is requested.
  INPUT GIVEN: bm,page
  OUTPUT: RC_BUFFER_STRATEGY_UNSPECIFIED;RC_OK; 
  UNIT TESTED BY: Surya
*/
RC insert_Into_QUEUE_Frontier(BM_BufferPool * const bm, BM_PageHandle * const page) {
    
    int freeFrame=0;
    Buffer_Strategy *buffer_Strategy;
    char *pagedata;
       
    struct Frontier_Queue *frontQUEUE = ((struct Frontier_Queue *) malloc (sizeof(struct Frontier_Queue)));
    
    buffer_Strategy = bm->mgmtData;
    
    if (first_Fifo == NULL && last_Fifo == NULL) {
        frontQUEUE->position = 0;
        frontQUEUE->isDirty = false;
        frontQUEUE->fixCount = 1;
  
        frontQUEUE->pageData=page->data;
        frontQUEUE->pageNumber = page->pageNum;
        first_Fifo = frontQUEUE;
        last_Fifo = first_Fifo;
        last_Fifo->nextNode = NULL;
	first_Fifo->nextNode = NULL;
	temp_Fifo = first_Fifo;
        buffer_Strategy->pageNumberVariable[frontQUEUE->position] = frontQUEUE->pageNumber;
        buffer_Strategy->fixedCountVariable[frontQUEUE->position] = frontQUEUE->fixCount;
        buffer_Strategy->dirtyPagesVariable[frontQUEUE->position] = frontQUEUE->isDirty;
        
        buffer_Strategy->isDataRead++;
    } else {
        
        int buffCount = 0;
	struct Frontier_Queue *temp_Frontier = first_Fifo;
	    while (temp_Frontier != NULL) {
		buffCount++;
		temp_Frontier = temp_Frontier->nextNode;
	    }
	
        if (bm->numPages == buffCount) {
            
            while (freeFrame != bm->numPages) {
                if (temp_Fifo->fixCount == 0) {
                    if (temp_Fifo->isDirty==true) {
                      
                        writeBlock(temp_Fifo->pageNumber, buffer_Strategy->fileHandle, temp_Fifo->pageData);
                        buffer_Strategy->isDataWrite++;
			
                    }
                    temp_Fifo->isDirty = false;
                    temp_Fifo->fixCount = 1;
                    temp_Fifo->pageData=page->data;
                    temp_Fifo->pageNumber = page->pageNum;
                    buffer_Strategy->pageNumberVariable[temp_Fifo->position] = temp_Fifo->pageNumber;
                    buffer_Strategy->fixedCountVariable[temp_Fifo->position] = temp_Fifo->fixCount;
                    buffer_Strategy->dirtyPagesVariable[temp_Fifo->position] = temp_Fifo->isDirty;
                    buffer_Strategy->isDataRead++;
                    break;
                } else {
                    if (temp_Fifo->nextNode == NULL) {
                        temp_Fifo = first_Fifo;
                    } else {
                        temp_Fifo = temp_Fifo->nextNode;
                    }
                    
                    freeFrame++;
                }
            }
            if (freeFrame == bm->numPages) {
                return RC_BUFFER_EXCEEDED;
            } else {
                if (temp_Fifo->nextNode == NULL) {
                    temp_Fifo = first_Fifo;
                } else {
                    temp_Fifo = temp_Fifo->nextNode;
                }
            }
        }
        else {
	    struct Frontier_Queue *newNode = ((struct Frontier_Queue *) malloc (sizeof(struct Frontier_Queue)));
            newNode->nextNode = NULL;
            newNode->position = last_Fifo->position + 1;
            newNode->isDirty = false;
            newNode->fixCount = 1;
            newNode->pageData=page->data;
            newNode->pageNumber = page->pageNum;
            last_Fifo->nextNode = newNode;
            last_Fifo = newNode;
            buffer_Strategy->pageNumberVariable[newNode->position] = newNode->pageNumber;
                    buffer_Strategy->fixedCountVariable[newNode->position] = newNode->fixCount;
                    buffer_Strategy->dirtyPagesVariable[newNode->position] = newNode->isDirty;
            buffer_Strategy->isDataRead++;
        }
    }
    return RC_OK;
}

/*
  IMPLEMENTED BY: Surya MODULE: update QUEUE Frontier when a new page is already in the buffer pool.
  INPUT GIVEN: bm,page
  OUTPUT: RC_BUFFER_STRATEGY_UNSPECIFIED;RC_OK; 
  UNIT TESTED BY: Raghu
*/
RC update_QUEUE_Frontier(struct Frontier_Queue *frontierQUEUE, Buffer_Strategy *buffer_Strategy, FrameMethodology pinValue, int position) {
       while (frontierQUEUE != NULL) {
        if (frontierQUEUE->position == position) {
            switch (pinValue) {
                case(MARK_Dirty):
                    frontierQUEUE->isDirty = true;
                    buffer_Strategy->dirtyPagesVariable[position] = frontierQUEUE->isDirty;
                    break;
                case(MARK_Pin):
                    frontierQUEUE->fixCount = frontierQUEUE->fixCount + 1;
                    buffer_Strategy->fixedCountVariable[position] = frontierQUEUE->fixCount;
                    break;
                case(MARK_Unpin):
                    frontierQUEUE->fixCount = frontierQUEUE->fixCount - 1;
                    buffer_Strategy->fixedCountVariable[position] = frontierQUEUE->fixCount;
                    break;
                case(MARK_NOT_Dirty):
                    frontierQUEUE->isDirty = false;
                    buffer_Strategy->dirtyPagesVariable[position] = frontierQUEUE->isDirty;
                    break;
            }
        }
        frontierQUEUE = frontierQUEUE->nextNode;
    }
    return RC_OK;
}

/*
  IMPLEMENTED BY: Raghu MODULE: Insert into Frontier when a new page is not in buffer pool for LRU.
  INPUT GIVEN: bm,page
  OUTPUT: RC_BUFFER_STRATEGY_UNSPECIFIED;RC_OK; 
  UNIT TESTED BY: Alekhya
*/
RC insert_Into_Frontier_LRU(BM_BufferPool * const bm, BM_PageHandle * const page) {

	Buffer_Strategy *buffer_Strategy;
	char *pagedata;
	int framePosition;
	int timeStamp_LRU;
	int bufferSize;
	int pos = 0;
	struct Frontier_LRU *frontier_LRU = ((struct Frontier_LRU *) malloc (sizeof(struct Frontier_LRU)));

	buffer_Strategy = bm->mgmtData;
	
	
	
	struct Frontier_LRU *tempFrontier_LRU;
	tempFrontier_LRU = lru_Frontier;
	bufferSize = 0;
	while (tempFrontier_LRU != NULL) {
		bufferSize++;
		tempFrontier_LRU = tempFrontier_LRU->nextNode;
	}

	if (lru_Frontier == NULL) {
		frontier_LRU->position = 0;
		frontier_LRU->timeStamp = 1;
		frontier_LRU->isDirty = false;
		frontier_LRU->fixCount = 1;
		frontier_LRU->pageData = page->data;
		frontier_LRU->pageNumber = page->pageNum;
		lru_Frontier = frontier_LRU;
		lru_Frontier->nextNode = NULL;
		buffer_Strategy->pageNumberVariable[0] = page->pageNum;

	} else {

		framePosition = get_FramePosition_LRU(page->pageNum);
		if (framePosition != -1) {
			frontier_LRU = lru_Frontier;
			while (frontier_LRU != NULL) {
				if (frontier_LRU->position == framePosition) {
					frontier_LRU->timeStamp = 1;
					frontier_LRU->isDirty = false;
					frontier_LRU->fixCount++;
					frontier_LRU->pageData = page->data;
					buffer_Strategy->pageNumberVariable[framePosition] = frontier_LRU->pageNumber;
					break;
				}
				frontier_LRU = frontier_LRU->nextNode;
			}

			tempFrontier_LRU = lru_Frontier;
	
			while (tempFrontier_LRU != NULL) {
				tempFrontier_LRU->timeStamp++;
				tempFrontier_LRU = tempFrontier_LRU->nextNode;
			}

	
		
		} else {
			
			if (bufferSize == bm->numPages) {
				tempFrontier_LRU = lru_Frontier;
				timeStamp_LRU = tempFrontier_LRU->timeStamp;
				framePosition = tempFrontier_LRU->position;
				while (tempFrontier_LRU != NULL) {
					if (tempFrontier_LRU->timeStamp > timeStamp_LRU) {
						timeStamp_LRU = tempFrontier_LRU->timeStamp;
						framePosition = tempFrontier_LRU->position;
					}
					tempFrontier_LRU = tempFrontier_LRU->nextNode;
				}
				printf("Frame Position LRU: %d", framePosition);
				
				tempFrontier_LRU = lru_Frontier;
				while (tempFrontier_LRU != NULL) {
					if (tempFrontier_LRU->position == framePosition) {
						if (tempFrontier_LRU->isDirty == true) {
							writeBlock(tempFrontier_LRU->pageNumber, buffer_Strategy->fileHandle, tempFrontier_LRU->pageData);
							buffer_Strategy->isDataWrite++;
						}
						tempFrontier_LRU->timeStamp = 1;
						tempFrontier_LRU->position = framePosition;
						tempFrontier_LRU->isDirty = false;
						tempFrontier_LRU->fixCount = 1;
						tempFrontier_LRU->pageData = page->data;
						tempFrontier_LRU->pageNumber = page->pageNum;
						buffer_Strategy->pageNumberVariable[framePosition] = page->pageNum;
						break;
					}
					tempFrontier_LRU = tempFrontier_LRU->nextNode;
				}
				
				tempFrontier_LRU = lru_Frontier;
	
				while (tempFrontier_LRU != NULL) {
					tempFrontier_LRU->timeStamp++;
					tempFrontier_LRU = tempFrontier_LRU->nextNode;
				}
				
				
			} else {
				tempFrontier_LRU = lru_Frontier;
				framePosition = tempFrontier_LRU->position; 
    				while (tempFrontier_LRU != NULL) {
					if (tempFrontier_LRU->position > framePosition) {
					    framePosition = tempFrontier_LRU->position;
					}
					tempFrontier_LRU = tempFrontier_LRU->nextNode;
    				}
	
			frontier_LRU->position = framePosition + 1;
			frontier_LRU->isDirty = false;
			frontier_LRU->fixCount = 1;
			
			tempFrontier_LRU = lru_Frontier;
			while (tempFrontier_LRU != NULL) {
				tempFrontier_LRU->timeStamp++;
				tempFrontier_LRU = tempFrontier_LRU->nextNode;
			}
			
			frontier_LRU->timeStamp = 1;
			frontier_LRU->pageData = page->data;
			frontier_LRU->pageNumber = page->pageNum;
			frontier_LRU->nextNode = lru_Frontier;
			lru_Frontier= frontier_LRU;
			buffer_Strategy->pageNumberVariable[frontier_LRU->position] = page->pageNum;

			
	
			}
		}
	}
	return RC_OK;

}


/*
  IMPLEMENTED BY: Alekhya MODULE: Insert into Frontier when a new page is not in buffer pool for Clock.
  INPUT GIVEN: bm,page
  OUTPUT: RC_BUFFER_STRATEGY_UNSPECIFIED;RC_OK; 
  UNIT TESTED BY: Surya
*/
RC insert_Into_Clock_Buffer(BM_BufferPool * const bm, BM_PageHandle * const page) {
   	Buffer_Strategy *buffer_Strategy;
	char *pagedata;
	int framePosition;
	int timeStamp_LRU;
	int bufferSize;
	int pos = 0;
	struct Frontier_Clock *frontier_Clock = ((struct Frontier_Clock *) malloc (sizeof(struct Frontier_Clock)));
	struct Frontier_Clock *frontier_Clock2 = ((struct Frontier_Clock *) malloc (sizeof(struct Frontier_Clock)));
	buffer_Strategy = bm->mgmtData;
     
    	frontier_Clock->pageData = (char *) malloc(PAGE_SIZE);
    if (first_Clock == NULL) {
        frontier_Clock->position = 0;
        frontier_Clock->isDirty = false;
        
        frontier_Clock->pageData = page->data;
        frontier_Clock->pageNumber = page->pageNum;
        frontier_Clock->reference = 1;
        frontier_Clock->fixCount = 1;
        first_Clock = frontier_Clock;
        first_Clock->nextNode = first_Clock;
        buffer_Strategy->pageNumberVariable[first_Clock->position] = first_Clock->pageNumber;
        tickle_Clock = first_Clock;
    } else {
        bufferSize = getBufferLength_Clock();
	
        if (bm->numPages == bufferSize) {
            frontier_Clock = tickle_Clock;
            while (framePosition != bm->numPages) {
	
                if (frontier_Clock->reference == 0 && frontier_Clock->fixCount == 0) {
			
                    if (frontier_Clock->isDirty) {
                        writeBlock(frontier_Clock->pageNumber, buffer_Strategy->fileHandle, frontier_Clock->pageData);
                        buffer_Strategy->isDataWrite++;
                    }

                    frontier_Clock->isDirty = false;
                    frontier_Clock->fixCount = 1;
                    
                    frontier_Clock->pageData=page->data;
                    frontier_Clock->pageNumber = page->pageNum;
                    buffer_Strategy->pageNumberVariable[frontier_Clock->position] = frontier_Clock->pageNumber;
                    buffer_Strategy->fixedCountVariable[frontier_Clock->position] = frontier_Clock->fixCount;
                    buffer_Strategy->dirtyPagesVariable[frontier_Clock->position] = frontier_Clock->isDirty;
                    break;
                } else if (frontier_Clock->reference == 0 && frontier_Clock->fixCount == 1) {
                    frontier_Clock = frontier_Clock->nextNode;
                } else if (frontier_Clock->reference == 1) {
                    frontier_Clock->reference = 0;
                    frontier_Clock = frontier_Clock->nextNode;
                }
                framePosition++;
            }
            if (framePosition == bm->numPages) {
                if (frontier_Clock == first_Clock) {
                    if (frontier_Clock->reference == 0 && frontier_Clock->fixCount == 0) {
                        if (frontier_Clock->isDirty) {
                            writeBlock(frontier_Clock->pageNumber, buffer_Strategy->fileHandle, frontier_Clock->pageData);
                            buffer_Strategy->isDataWrite++;
                        }

                        frontier_Clock->isDirty = false;
                        frontier_Clock->fixCount = 1;
                        
                        frontier_Clock->pageData=page->data;
                        frontier_Clock->pageNumber = page->pageNum;
                        buffer_Strategy->pageNumberVariable[frontier_Clock->position] = frontier_Clock->pageNumber;
                        buffer_Strategy->fixedCountVariable[frontier_Clock->position] = frontier_Clock->fixCount;
                        buffer_Strategy->dirtyPagesVariable[frontier_Clock->position] = frontier_Clock->isDirty;
                        tickle_Clock = frontier_Clock->nextNode;
                    } else {
                        return RC_BUFFER_EXCEEDED;
                    }
                } else {
                    return RC_BUFFER_EXCEEDED;
                }
            } else {
                tickle_Clock = frontier_Clock->nextNode;
            }
        } else {
	
            frontier_Clock2 = first_Clock;
            while (frontier_Clock2->nextNode != first_Clock) {
               frontier_Clock2 = frontier_Clock2->nextNode;
		
            }
		
		
	    frontier_Clock->position = frontier_Clock2->position + 1;
            frontier_Clock->isDirty = false;
		
            frontier_Clock->pageData = page->data;
            frontier_Clock->pageNumber = page->pageNum;
            frontier_Clock->reference = 1;
            frontier_Clock->fixCount = 0;
	    frontier_Clock2->nextNode = frontier_Clock;
            frontier_Clock->nextNode = first_Clock;
            buffer_Strategy->pageNumberVariable[frontier_Clock->position] = frontier_Clock->pageNumber;
            buffer_Strategy->fixedCountVariable[frontier_Clock->position] = frontier_Clock->fixCount;
            buffer_Strategy->dirtyPagesVariable[frontier_Clock->position] = frontier_Clock->isDirty;
		
        }
    }
    return RC_OK;
}



/*
  IMPLEMENTED BY: Surya MODULE: get the current total number of frames in the Buffer Pool.
  INPUT GIVEN: bm
  OUTPUT: PageNumber array
  UNIT TESTED BY: Raghu
*/
PageNumber *getFrameContents (BM_BufferPool *const bm){
	Buffer_Strategy *buffer_Strategy = ((Buffer_Strategy *) malloc (sizeof(Buffer_Strategy)));
	buffer_Strategy = bm->mgmtData;

    return buffer_Strategy->pageNumberVariable;
}

/*
  IMPLEMENTED BY: Raghu MODULE: get all the dirty page frames in the Buffer Pool.
  INPUT GIVEN: bm
  OUTPUT: array list of dirty bit
  UNIT TESTED BY: Alekhya
*/
bool *getDirtyFlags (BM_BufferPool *const bm)
{
	Buffer_Strategy *buffer_Strategy = ((Buffer_Strategy *) malloc (sizeof(Buffer_Strategy)));
	buffer_Strategy = bm->mgmtData;

    return buffer_Strategy->dirtyPagesVariable;
}

/*
  IMPLEMENTED BY: Alekhya MODULE: get the fixed counts of dirty page frames in the Buffer Pool.
  INPUT GIVEN: bm
  OUTPUT: array list of Integer
  UNIT TESTED BY: Surya
*/
int *getFixCounts (BM_BufferPool *const bm)
{
	Buffer_Strategy *buffer_Strategy = ((Buffer_Strategy *) malloc (sizeof(Buffer_Strategy)));
	buffer_Strategy = bm->mgmtData;

    return buffer_Strategy->fixedCountVariable;
}

/*
  IMPLEMENTED BY: Surya MODULE: get if the count of the page requested read in the Buffer Pool.
  INPUT GIVEN: bm
  OUTPUT: Integer
  UNIT TESTED BY: Raghu
*/
int getNumReadIO (BM_BufferPool *const bm)
{
	Buffer_Strategy *buffer_Strategy = ((Buffer_Strategy *) malloc (sizeof(Buffer_Strategy)));
	buffer_Strategy = bm->mgmtData;

    return buffer_Strategy->isDataRead;
}

/*
  IMPLEMENTED BY: Raghu MODULE: get if the count of the page requested write in the Buffer Pool.
  INPUT GIVEN: bm
  OUTPUT: Integer
  UNIT TESTED BY: Alekhya
*/
int getNumWriteIO (BM_BufferPool *const bm)
{
	Buffer_Strategy *buffer_Strategy = ((Buffer_Strategy *) malloc (sizeof(Buffer_Strategy)));
	buffer_Strategy = bm->mgmtData;

    return buffer_Strategy->isDataWrite;
}


/*
  IMPLEMENTED BY: Alekhya MODULE: Search if the page frame already exist in QUEUE Frontier and return the position.
  INPUT GIVEN: PageNumber
  OUTPUT: Integer
  UNIT TESTED BY: Surya
*/
int search_QUEUE_Frontier(PageNumber pageNumber)
{
    
	int framePosition = -1;
	struct Frontier_Queue *tempFrontier  = first_Fifo;
    	while (tempFrontier != NULL) {
        if (tempFrontier->pageNumber == pageNumber) {
            framePosition = tempFrontier->position;
            break;
        }
        tempFrontier = tempFrontier->nextNode;
    }
    return framePosition;

}

/*
  IMPLEMENTED BY: Surya MODULE: Search if the page frame already exist in LRU Frontier and return the position.
  INPUT GIVEN: PageNumber
  OUTPUT: Integer
  UNIT TESTED BY: Raghu
*/
int get_FramePosition_LRU(PageNumber pageNumber)
{
	int framePosition = -1;
	struct Frontier_LRU *frontier_LRU;
	frontier_LRU = lru_Frontier;
	while (frontier_LRU != NULL) {
		if (frontier_LRU->pageNumber == pageNumber) {
			framePosition = frontier_LRU->position;
			break;
		}
		frontier_LRU = frontier_LRU->nextNode;
	}
	return framePosition;
}

/*
  IMPLEMENTED BY: Raghu MODULE: Search if the page frame already exist in Clock Frontier and return the position.
  INPUT GIVEN: PageNumber
  OUTPUT: Integer
  UNIT TESTED BY: Alekhya
*/
int getFrameClockPosition(PageNumber pageNumber)
{
	  
    int position = -1;
    struct Frontier_Clock * frontier_Clock = (struct Frontier_Clock *) malloc(sizeof (struct Frontier_Clock));

    frontier_Clock = first_Clock;
    if (first_Clock != NULL) {
        if (frontier_Clock->nextNode == first_Clock && first_Clock->pageNumber==pageNumber) {
            position = first_Clock->position;
        } else {
            while (frontier_Clock->nextNode != first_Clock) {
                if (frontier_Clock->pageNumber == pageNumber) {
                    position = frontier_Clock->position;
                    break;
                }
                frontier_Clock = frontier_Clock->nextNode;
            }
        }
    }
    return position;
}


/*
  IMPLEMENTED BY: Alekhya MODULE: get the total length of CLOCK Buffer Pool.
  INPUT GIVEN: VOID
  OUTPUT: Integer Count
  UNIT TESTED BY: Surya
*/
int getBufferLength_Clock() {
	struct Frontier_Clock * frontier_Clock;
    int count = 0;
    frontier_Clock = (struct Frontier_Clock *) malloc(sizeof (struct Frontier_Clock));
    frontier_Clock = first_Clock;
    if (first_Clock != NULL) {
        if (frontier_Clock->nextNode == first_Clock) {
            return 1;
        } else {
            while (frontier_Clock-> nextNode != first_Clock) {
                count++;
                frontier_Clock = frontier_Clock->nextNode;
            }
        }
    }
    return count + 1;

}

