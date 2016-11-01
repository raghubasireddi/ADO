//Header Section

#include "storage_mgr.h"
#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include <time.h>
#include "buffer_mgr.h"
#include "buffer_mgr_stat.h"
#include "dberror.h"
#include "dt.h"

//End Header Section

//Struct Declarations

struct LRU_buffer {
    int pos;
    int pageNum;
    int age;
    bool dirty;
    int fixCount;
    char *data;
    struct LRU_buffer *next;
} *head_lru = NULL;

struct FIFO_buffer {
    int pos;
    int pageNum;
    int fixCount;
    bool dirty;
    char *data;
    struct FIFO_buffer *next;
} *head_fifo = NULL, *tail_fifo = NULL, *handler = NULL;

typedef struct bufferpool_stat {
    int readIO;
    int writeIO;
    int *fixCounts_stat;
    PageNumber *pageNumber_stat;
    bool *dirtyPages_stat;
    void *buf;
    SM_FileHandle *fHandle;
    struct bufferpool_stat *pool_stat;
} bufferpool_stat;

//End Struct Declaration

//Macros

#define MAKE_LRU_BUFFER()                                       \
  ((struct LRU_buffer *) malloc (sizeof(struct LRU_buffer)))

#define MAKE_FIFO_BUFFER()                              \
  ((struct FIFO_buffer *) malloc (sizeof(struct FIFO_buffer)))

#define MAKE_BUFFERPOOL_STAT()                          \
  ((bufferpool_stat *) malloc (sizeof(bufferpool_stat)))

//LRU Page replacement methods

/*
 *  * AUTHOR: Prajwal
 *  * Language: C
 *  * Section: bufferLength
 *  * Purpose: Calculate Length of Buffer
 *  * INPUT: NONE
 *  * OUTPUT: Length of buffer
 *  */

int bufferLength() {
    struct LRU_buffer *node;
    node = head_lru;
    int count = 0;
    while (node != NULL) {
        count++;
        node = node->next;
    }
    return count;
}

//End bufferLength Section


/*
 *  * AUTHOR: Prajwal
 *  * Language: C
 *  * Section: searchFrame_LRU
 *  * Purpose: Search for Frame
 *  * INPUT: PageNUmber
 *  * OUTPUT: Page Frame Position
 *  */

int searchFrame_LRU(PageNumber pNum) {
    int flag = -1;
    struct LRU_buffer *temp;
    temp = head_lru;
    while (temp != NULL) {
        if (temp->pageNum == pNum) {
            flag = temp->pos;
            break;
        }
        temp = temp->next;
    }
    return flag;
}

//End searchFrame_LRU Section

/*
 *  * AUTHOR: Prajwal
 *  * Language: C
 *  * Section: LRU
 *  * Purpose: LRU algorithm
 *  * INPUT: NONE
 *  * OUTPUT: position
 *  */

int LRU() {
    struct LRU_buffer *temp;
    temp = head_lru;
    int max_pos;
    int max_age;
    max_age = temp->age;
    max_pos = temp->pos;
    while (temp != NULL) {
        if (temp->age > max_age) {
            max_age = temp->age;
            max_pos = temp->pos;
        }
        temp = temp->next;
    }
    return max_pos;
}

//End LRU Section

/*
 *  * AUTHOR: Prajwal
 *  * Language: C
 *  * Section: increment_age
 *  * Purpose: increment age of the node each time it is revisted
 *  * INPUT: NONE
 *  * OUTPUT: RC_OK-SUCESS
 *  */
RC increment_age() {
    struct LRU_buffer *node;
    node = head_lru;
    while (node != NULL) {
        node->age = node->age + 1;
        node = node->next;
    }
    return RC_OK;
}

//End IncrementAge Section

/*
 *
 *  * AUTHOR  : Prajwal
 *  * Language: C
 *  * Section : maxPos
 *  * METHOD  : present Position of the Buffer(Last Value)
 *  * INPUT   : NONE
 *  * OUTPUT  : The maximum position of the buffer
 *  */

int maxpos() {

    struct LRU_buffer *temp;
    temp = head_lru;
    int max;
    max = temp->pos;
    while (temp != NULL) {

        if (temp->pos > max) {
            max = temp->pos;
        }
        temp = temp->next;
    }
    return max;
}

//End maxPos Section

/*
 *  * AUTHOR: Prajwal
 *  * Language: C
 *  * Section: insert_LRU_buffer
 *  * Purpose: insert into LRU buffer
 *  * INPUT: File Structure
 *  * OUTPUT: RC_OK
 *  */

RC insert_LRU_buffer(BM_BufferPool *const bm,BM_PageHandle *const page)
{

    struct LRU_buffer *temp, *temp1;
    bufferpool_stat *pool_stat;
    pool_stat = bm->mgmtData;

    temp = MAKE_LRU_BUFFER();
    temp1 = MAKE_LRU_BUFFER();

    int search;
    int sizeOfBuffer;
    int pos = 0;
    sizeOfBuffer = bufferLength();

    if (head_lru == NULL) {

        temp->pos = 0;
        temp->pageNum = page->pageNum;
        temp->age = 1;
        temp->fixCount = 1;
        temp->dirty = false;
        temp->data = page->data;

        head_lru = temp;
        head_lru->next = NULL;
    pool_stat->pageNumber_stat[temp->pos] = temp->pageNum;

    }

    else {

           search = searchFrame_LRU(page->pageNum);
           if (search != NO_PAGE)
              {
                 temp = head_lru;
                 while (temp != NULL)
                 {
                    if (temp->pos == search)
                   {
                    break;
                   }
                 temp = temp->next;
                 }

                 increment_age();
                 temp->age = 1;
                 temp->pos= search;
                 temp->dirty = false;
                 temp->fixCount = temp->fixCount + 1;
                 temp->data = page->data;

                }

          else
            {
                if (sizeOfBuffer == bm->numPages)
                {
                  pos = LRU();
                  temp = head_lru;
                  while (temp != NULL)
                   {
                     if (temp->pos == pos)
                     {
                       break;
                     }
                    temp = temp->next;
                  }

                  if (temp->dirty == true)
                  {
                    writeBlock(temp->pageNum, pool_stat->fHandle, temp->data);
                    pool_stat->writeIO = pool_stat->writeIO + 1;
                  }

                  increment_age();

                  temp->age = 1;
                  temp->pos = pos;
                  temp->dirty = false;
                  temp->fixCount = 1;

                  temp->data = page->data;
                  temp->pageNum = page->pageNum;

                  pool_stat->pageNumber_stat[pos] = temp->pageNum;
                 }

                else
                {
                 temp1 = head_lru;
                while (temp1 != NULL) {
                    pos = temp1->pos;
                    temp1 = temp1->next;
                }

                    pos = maxpos();
                    temp->pos = pos + 1;
                    temp->dirty = false;
                    temp->fixCount = 1;
                    increment_age();
                    temp->age = 1;
                    temp->data = page->data;
                    temp->pageNum = page->pageNum;
                    temp->next = head_lru;                                                                                                                                                       head_lru = temp;                                                                                                                                                       pool_stat->pageNumber_stat[temp->pos] = temp->pageNum;
            }
        }
    }
    return RC_OK;

}

//End insertLRUBuffer Section

//FIFO Page replacement method

/*
 *  * AUTHOR  : Meenakshi
 *  * Language: C
 *  * Section : length_buffer
 *  * METHOD  : find Length of FIFO Buffer
 *  * INPUT   : NONE
 *  * OUTPUT  : int (length of the buffer)
 *  */

int length_buffer()
{
    struct FIFO_buffer *temp;
    int count = 0;
    temp = MAKE_FIFO_BUFFER();
    temp = head_fifo;
    while (temp != NULL) {
        count++;
        temp = temp->next;
    }
    return count;
}
//End Length_buffer

/*
 * AUTHOR  : Meenakshi
 * * Language: C
 * * Section : insert_frame_fifo_buffer
 * * METHOD  : Insert new frame in the Fifo Buffer
 * * INPUT   : BM_BufferPool,BM_PageHandle
 * * OUTPUT  : RC_OK-SUCESSS;RC_BUFFER_EXCEEDED
 */

RC insert_frame_fifo_buffer(BM_BufferPool * const bm, BM_PageHandle * const page) {
    int buf_size_current;
    int free_frame_current=0;
    char *pagedata;
    struct FIFO_buffer *temp, *temp1;
    bufferpool_stat *pool_stat;
    temp = MAKE_FIFO_BUFFER();
    temp1= MAKE_FIFO_BUFFER();
    pool_stat = bm->mgmtData;
    if (head_fifo == NULL && tail_fifo == NULL) {
        temp->pos = 0;
        temp->dirty = false;
        temp->fixCount = 1;
        temp->data=page->data;
        temp->pageNum = page->pageNum;
        tail_fifo = temp;
        head_fifo = temp;
        head_fifo->next = tail_fifo;
        tail_fifo->next = NULL;
        pool_stat->pageNumber_stat[temp->pos] = temp->pageNum;
        pool_stat->fixCounts_stat[temp->pos] = temp->fixCount;
        pool_stat->dirtyPages_stat[temp->pos] = temp->dirty;
        handler = head_fifo;
        pool_stat->readIO++;
    } else {

        buf_size_current = length_buffer();
        if (bm->numPages == buf_size_current) {

            while (free_frame_current != bm->numPages) {
                if (handler->fixCount == 0) {
                    if (handler->dirty==true) {

                        writeBlock(handler->pageNum, pool_stat->fHandle, handler->data);
                        pool_stat->writeIO++;
                    }
                    handler->dirty = false;
                   handler->fixCount = 1;

                    handler->data=page->data;
                    handler->pageNum = page->pageNum;
                    pool_stat->pageNumber_stat[handler->pos] = handler->pageNum;
                    pool_stat->fixCounts_stat[handler->pos] = handler->fixCount;
                    pool_stat->dirtyPages_stat[handler->pos] = handler->dirty;
                    pool_stat->readIO++;
                    break;
                } else {
                    if (handler->next == NULL) {
                        handler = head_fifo;
                    } else {
                        handler = handler->next;
                    }

                    free_frame_current++;
                }
            }
            if (free_frame_current == bm->numPages) {
                return RC_BUFFER_EXCEEDED;
            } else {
                if (handler->next == NULL) {
                    handler = head_fifo;
                } else {
                    handler = handler->next;
                }
            }
        }
        else {
            temp->next = NULL;
            temp->pos = tail_fifo->pos + 1;
            temp->dirty = false;
            temp->fixCount = 1;

            temp->data=page->data;
            temp->pageNum = page->pageNum;
            tail_fifo->next = temp;
            tail_fifo = temp;
            pool_stat->pageNumber_stat[temp->pos] = temp->pageNum;
            pool_stat->fixCounts_stat[temp->pos] = temp->fixCount;
            pool_stat->dirtyPages_stat[temp->pos] = temp->dirty;
            pool_stat->readIO++;
        }
    }
    return RC_OK;
}
//End Insert_Frame_FIFO_Buffer

/*
 *  * AUTHOR  : Meenakshi
 *  * Language: C
 *  * Section : search_frame_fifo_buffer
 *  * METHOD  : Search for a frame in the Fifo Buffer
 *  * INPUT   : PageNumber
 *  * OUTPUT  : Page Position in Frame
 *  */

int search_frame_fifo_buffer(PageNumber pNum) {
    struct FIFO_buffer *temp;
    int pos = -1;
    temp = MAKE_FIFO_BUFFER();
    temp = head_fifo;
    while (temp != NULL) {
        if (temp->pageNum == pNum) {
            pos = temp->pos;
            break;
        }
        temp = temp->next;
    }
    return pos;
}

//End search_frame_fifo_buffer

/*
 * * AUTHOR  : Meenakshi
 * * Language: C
 * * Section : update_frame_stat_fifo_buffer
 * * METHOD  : Updating frame status in FIFO Buffer
 * * INPUT   : FIFO_buffer,bufferpool_stat,UpdateFrameData,Position
 * * OUTPUT  : RC_OK-SUCESSS;RC_OTHERS-ON FAIL;
 * */

RC update_frame_stat_fifo_buffer(struct FIFO_buffer *buf, bufferpool_stat *pool_stat, UpdateFrameData update_value, int pos) {
    while (buf != NULL) {
        if (buf->pos == pos) {
            switch (update_value) {
                case(UP_Dirty):
                    buf->dirty = true;
                    pool_stat->dirtyPages_stat[pos] = buf->dirty;
                    break;
                case(UP_Pin):
                    buf->fixCount = buf->fixCount + 1;
                    pool_stat->fixCounts_stat[pos] = buf->fixCount;
                   break;
                case(UP_Unpin):
                    buf->fixCount = buf->fixCount - 1;
                    pool_stat->fixCounts_stat[pos] = buf->fixCount;
                    break;
                case(UP_NOT_Dirty):
                    buf->dirty = false;
                    pool_stat->dirtyPages_stat[pos] = buf->dirty;
                    break;
            }
        }
        buf = buf->next;
    }
    return RC_OK;
}

//End Frame_stat-fifo_buffer

/*
 *  * AUTHOR: Prajwal
 *  * Language: C
 *  * Section: initBufferPool
 *  * METHOD: Initialise Buffer Pool
 *  * INPUT: Buffer POOL;PAGE FRAME;BUFFER SIZE;REPLACEMENT STRATEGY;STRAT DATA
 *  * OUTPUT: RC_OK-SUCESSS;RC_OTHERS-ON FAIL;
 *  */

RC initBufferPool(BM_BufferPool * const bm, const char *const pageFileName, const int numPages, ReplacementStrategy strategy, void* stratData) {

    SM_FileHandle *fHandle = (SM_FileHandle *) malloc(PAGE_SIZE);
    memset(fHandle,0,PAGE_SIZE);
    bufferpool_stat *pool_stat;

    struct FIFO_buffer *temp;
    PageNumber* pageNumArray;
    int* fixCountsArray;
    bool* dirtyMarked;
    int i = 0;

    temp = MAKE_FIFO_BUFFER();
    pool_stat = MAKE_BUFFERPOOL_STAT();

    bm->pageFile = (char *) pageFileName;
    bm->numPages = numPages;
    bm->strategy = strategy;
    pageNumArray = (PageNumber *) malloc(sizeof (PageNumber) * bm->numPages);
    fixCountsArray = (int *) malloc(sizeof (int)*numPages);
    dirtyMarked = (bool *) malloc(sizeof (bool) * numPages);
    for (i = 0; i < bm->numPages; i++) {
        pageNumArray[i] = NO_PAGE;
        fixCountsArray[i] = 0;
        dirtyMarked[i] = false;
    }
    pool_stat->readIO = 0;
    pool_stat->writeIO = 0;
    pool_stat->pageNumber_stat = pageNumArray;
    pool_stat->dirtyPages_stat = dirtyMarked;
    pool_stat->fixCounts_stat = fixCountsArray;


    openPageFile(bm->pageFile, fHandle);
    switch (strategy) {
        case RS_FIFO:
            pool_stat->fHandle = fHandle;
            pool_stat->buf = head_fifo;
            break;

        case RS_LRU:
            pool_stat->fHandle = fHandle;
            pool_stat->buf = head_lru;
            break;

        default:
            return RC_STRATEGY_UNDEFINED;
    }
    bm->mgmtData = pool_stat;
    return RC_OK;
}

//End InitBufferpool Section

/*
 *  * AUTHOR: Prajwal
 *  * Language: C
 *  * Section: shutdownBufferPool
 *  * Purpose: Destroys Buffer Pool
 *  * INPUT: BM_BufferPool
 *  * OUTPUT: RC_OK-SUCESSS;RC_OTHERS-ON FAIL;
 *  */
RC shutdownBufferPool(BM_BufferPool * const bm) {
    bufferpool_stat *pool_stat;
    struct FIFO_buffer *temp;
    struct LRU_buffer *temp1;
    temp = MAKE_FIFO_BUFFER();
    temp1 = MAKE_LRU_BUFFER();

    pool_stat = bm->mgmtData;

    int pos = 0;
    int numofPages = bm->numPages;

    switch (bm->strategy) {
        case RS_FIFO:
            temp = head_fifo;
            while (temp != NULL) {
                if (temp->dirty) {
                    writeBlock(temp->pageNum, pool_stat->fHandle, temp->data);
                    pool_stat->writeIO = pool_stat->writeIO + 1;
                }
                temp = temp->next;
            }
            break;

        case RS_LRU:
            temp1 = head_lru;
            while (temp1 != NULL) {
                if (temp1->dirty) {
                    writeBlock(temp1->pageNum, pool_stat->fHandle, temp1->data);
                    pool_stat->writeIO = pool_stat->writeIO + 1;
                }
                temp1 = temp1->next;
            }
            break;
        default:
            return RC_STRATEGY_UNDEFINED;
    }

    head_fifo = NULL;
    tail_fifo = NULL;
    handler = NULL;
    head_lru = NULL;
    closePageFile(pool_stat->fHandle);
    free(temp);
    free(temp1);
    free(pool_stat->dirtyPages_stat);
    free(pool_stat->fixCounts_stat);
    free(pool_stat->pageNumber_stat);
    free(pool_stat->fHandle);
   free(pool_stat->buf);

    return RC_OK;
}

//End ShutDownBufferPool Section
/*
 *  * AUTHOR: Meenakshi
 *  * Language: C
 *  * Section: forceFlushPool
 *  * Purpose: Dirty pages are written back to disk
 *  * INPUT: BM_BufferPool
 *  * OUTPUT: RC_OK-SUCESSS;RC_OTHERS-ON FAIL;
 *  */

RC forceFlushPool(BM_BufferPool * const bm) {

    bufferpool_stat *pool_stat;
    struct LRU_buffer *temp1;
    struct FIFO_buffer *temp;
    int numberofPages = bm->numPages;
    int pos;
    temp = MAKE_FIFO_BUFFER();
    temp1 = MAKE_LRU_BUFFER();

    pool_stat = bm->mgmtData;

    switch (bm->strategy) {
         case RS_FIFO:
            temp = head_fifo;
            while (temp != NULL) {
                if (temp->dirty) {
                    writeBlock(temp->pageNum, pool_stat->fHandle, temp->data);
                    pool_stat->writeIO++;
                    temp->dirty = false;
                    pool_stat->dirtyPages_stat[temp->pos] = temp->dirty;
                }
                temp = temp->next;
            }
            break;
        case RS_LRU:
            temp1 = head_lru;
            while (temp1 != NULL) {
                if (temp1->dirty) {
                    writeBlock(temp1->pageNum, pool_stat->fHandle, temp1->data);
                    pool_stat->writeIO = pool_stat->writeIO + 1;
               }
                temp1 = temp1->next;
            }
            break;
        default:
            return RC_STRATEGY_UNDEFINED;
    }
    free(temp);
    free(temp1);
    return RC_OK;
}

//End ForceFlushpool Section

/*
 *  * AUTHOR: Meenakshi
 *  * Language: C
 *  * Section: pinPage
 *  * Purpose: Pin the Pages with page numbers
 *  * INPUT: BM_BufferPool,BM_PageHandle,PageNumber
 *  * OUTPUT: RC_OK-SUCESSS;RC_OTHERS-ON FAIL;
 *  */

RC pinPage(BM_BufferPool * const bm, BM_PageHandle * const page, const PageNumber pageNum) {
    SM_PageHandle ph;

    struct LRU_buffer *temp;
    struct LRU_buffer *node;
    bufferpool_stat *pool_stat;
    int pos = -1;

    ph = (SM_PageHandle) malloc(PAGE_SIZE);
    memset(ph,0,PAGE_SIZE);
    temp = MAKE_LRU_BUFFER();
    node = MAKE_LRU_BUFFER();


    pool_stat = bm->mgmtData;
    node = pool_stat->buf;
    page->pageNum = pageNum;
    switch (bm->strategy) {
        case RS_FIFO:
            pos = search_frame_fifo_buffer(page->pageNum);
            if(pos ==-1)
            {
             readBlock(pageNum, pool_stat->fHandle, ph);
             page->data=ph;

            }
            if (pos != -1) {
                update_frame_stat_fifo_buffer(head_fifo, pool_stat, UP_Pin, pos);
            } else {
                insert_frame_fifo_buffer(bm, page);
            }
            break;
        case RS_LRU:
             if(searchFrame_LRU(page->pageNum)==-1){
             readBlock(pageNum, pool_stat->fHandle, ph);
             pool_stat->readIO = pool_stat->readIO+1;
             page->data=ph;
            }
            insert_LRU_buffer(bm,page);
            break;
        default:
            return RC_STRATEGY_UNDEFINED;
    }
    free(temp);
    free(node);
    return RC_OK;
}

//End pinPage Sectin

/*
 *  * AUTHOR: Meenakshi
 *  * Language: C
 *  * Section: markDirty
 *  * Purpose: mark the page as Dirty
 *  * INPUT: BM_BufferPool,BM_PageHandle
 *  * OUTPUT: RC_OK-SUCESSS;RC_OTHERS-ON FAIL;
 *  */
RC markDirty(BM_BufferPool * const bm, BM_PageHandle * const page) {
    bufferpool_stat *pool_stat;
    struct LRU_buffer *node;
    struct FIFO_buffer *buf;
    int pos = -1;
    buf = MAKE_FIFO_BUFFER();

    node = MAKE_LRU_BUFFER();

    pool_stat = bm->mgmtData;


    switch (bm->strategy) {
       case RS_FIFO:
            pos = search_frame_fifo_buffer(page->pageNum);
            buf = head_fifo;
            if (pos != -1) {
                while (buf != NULL) {
                    if (buf->pos == pos) {

                        buf->data=page->data;
                        update_frame_stat_fifo_buffer(head_fifo,pool_stat, UP_Dirty, pos);
                    }
                    buf = buf->next;
                }
            }
            break;
        case RS_LRU:
          pos = searchFrame_LRU(page->pageNum);
            node = head_lru;
            while (node != NULL) {
                if (node->pos == pos) {
                    node->dirty = true;
                    pool_stat->dirtyPages_stat[pos] = node->dirty;
                }
                node = node->next;
            }
            break;
        default:
            return RC_STRATEGY_UNDEFINED;
    }

    return RC_OK;
}

//End MarkDirty Section
/*
 *  * AUTHOR: Meenakshi
 *  * Language: C
 *  * Section: unpinPage
 *  * Purpose: Unpins the  page 
 *  * INPUT: BM_BufferPool,BM_PageHandle
 *  * OUTPUT: RC_OK-SUCESSS;RC_OTHERS-ON FAIL;
 *  */

RC unpinPage(BM_BufferPool * const bm, BM_PageHandle * const page) {
    struct FIFO_buffer *bufFIFO;
    struct LRU_buffer *bufLRU;
    bufferpool_stat *pool_stat;
    int pos = -1;
    int pinCount = 0;
    bufFIFO = MAKE_FIFO_BUFFER();
    bufLRU = MAKE_LRU_BUFFER();

    pool_stat = bm->mgmtData;

    switch (bm->strategy) {
        case RS_FIFO:
            pos = search_frame_fifo_buffer(page->pageNum);
            bufFIFO = head_fifo;
            while (bufFIFO != NULL) {
                if (bufFIFO->pos == pos) {
                    update_frame_stat_fifo_buffer(head_fifo, pool_stat, UP_Unpin, pos);
                }
                bufFIFO = bufFIFO->next;
            }
            break;
        case RS_LRU:
            pos = searchFrame_LRU(page->pageNum);
            bufLRU = head_lru;
            while (bufLRU != NULL) {
                if (bufLRU->pos == pos) {
                    bufLRU->fixCount = bufLRU->fixCount - 1;
                    pool_stat->fixCounts_stat[bufLRU->pos] = bufLRU->fixCount;
                }
                bufLRU = bufLRU->next;
            }
            break;
        default:
            return RC_STRATEGY_UNDEFINED;
    }
    return RC_OK;
}

//End Unpin Section
/*
 *  * AUTHOR: Meenakshi
 *  * Language: C
 *  * Section: forcePage
 *  * Purpose: write date to disk
 *  * INPUT: BM_BufferPool,BM_PageHandle
 *  * OUTPUT: RC_OK-SUCESSS;RC_OTHERS-ON FAIL;
 *  */

RC forcePage(BM_BufferPool * const bm, BM_PageHandle * const page) {
    bufferpool_stat *pool_stat;
    struct FIFO_buffer *temp;
    struct LRU_buffer *bufLru;

    bufLru = MAKE_LRU_BUFFER();
    temp = MAKE_FIFO_BUFFER();

    pool_stat = bm->mgmtData;


    int pos = -1;
    writeBlock(page->pageNum, pool_stat->fHandle, page->data);

    switch (bm->strategy) {
        case RS_FIFO:
            pos = search_frame_fifo_buffer(page->pageNum);
            temp = head_fifo;
            if (pos != -1) {
                while (temp != NULL) {
                    if (temp->pos == pos) {
                        temp->data=page->data;
                        writeBlock(temp->pageNum, pool_stat->fHandle, temp->data);
                        pool_stat->writeIO++;
                        update_frame_stat_fifo_buffer(pool_stat->buf, pool_stat, UP_NOT_Dirty, pos);
                    }
                    temp = temp->next;
                }
            }
           break;
       case RS_LRU:
            pos = searchFrame_LRU(page->pageNum);
            bufLru = head_lru;
            if (pos != -1) {
                while (bufLru != NULL) {
                    if (bufLru->pos == pos) {
                        bufLru->data=page->data;
                        writeBlock(bufLru->pageNum, pool_stat->fHandle, bufLru->data);
                        bufLru->dirty = false;
                        pool_stat->dirtyPages_stat[pos] = bufLru->dirty;
                        pool_stat->writeIO++;
                    }
                    bufLru = bufLru->next;
                }
            }
            break;
        default:
            return RC_STRATEGY_UNDEFINED;
    }
    return RC_OK;
}

//End forcePage Section

/*
 *  * AUTHOR: Prajwal
 *  * Language: C
 *  * Section: getFrameContents
 *  * Purpose: returns an array of PageNumbers
 *  * INPUT: BM_BufferPool 
 *  * OUTPUT: PageNumber 
 *  */

PageNumber *getFrameContents(BM_BufferPool * const bm) {

    bufferpool_stat *pool_stat;

    pool_stat = bm->mgmtData;

    return pool_stat->pageNumber_stat;
}

//End getFrameContents Section
int *getFixCounts(BM_BufferPool * const bm) {

    bufferpool_stat *pool_stat;

    pool_stat = bm->mgmtData;

    return pool_stat->fixCounts_stat;

}

//End getFixCounts Section

/*
 *  * AUTHOR: Prajwal
 *  * Language: C
 *  * Section: getDirtyFlags
 *  * Purpose: dirty marked statistics; returns bool value
 *  * INPUT: BM_BufferPool
 *  * OUTPUT: bool *
 *  */

bool *getDirtyFlags(BM_BufferPool * const bm) {

    bufferpool_stat *pool_stat;

    pool_stat = bm->mgmtData;

    return pool_stat->dirtyPages_stat;
}

//End getDirtyFlags Section

/*
 *  * AUTHOR: Prajwal
 *  * Language: C
 *  * Section: getNumReadIO
 *  * METHOD: get Number of read from Disk
 *  * INPUT: BM_BufferPool
 *  * OUTPUT: int
 *  */
int getNumReadIO(BM_BufferPool * const bm) {

    bufferpool_stat *pool_stat;
    pool_stat = bm->mgmtData;
    return pool_stat->readIO;
}

//End getNumReadIO Section

/*
 *  * AUTHOR: Prajwal
 *  * Language: C
 *  * Section: getNumWriteIO
 *  * Purpose: get Number of writes to disk
 *  * INPUT: BM_BufferPool
 *  * OUTPUT: int
 *  */

int getNumWriteIO(BM_BufferPool * const bm) {
    bufferpool_stat *pool_stat;

   pool_stat = bm->mgmtData;

    return pool_stat->writeIO;
}

//End getNumWriteIO Section




