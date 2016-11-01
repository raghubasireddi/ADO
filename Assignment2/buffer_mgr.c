#include "storage_mgr.h"
#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include <time.h>
#include "buffer_mgr.h"
#include "buffer_mgr_stat.h"
#include "dberror.h"
#include "dt.h"

struct buffer_LRU {
    int pos;
    int pageNum;
    int fixCount;
    bool dirty;
    int age;
    char *data;
    struct buffer_LRU *next;
} *start_lru = NULL;
struct buffer_Queue {
    int pos;
    int pageNum;
    int fixCount;
    bool dirty;
    char *data;
    struct buffer_Queue *next;
} *strt_fifo = NULL, *rear_fifo = NULL, *handler = NULL;

typedef struct buffer_pool_stat {
    int readIO;
    int writeIO;
    PageNumber *pageNumber_stat;
    int *fixCounts_stat;
    bool *dirtyPages_stat;
} buffer_pool_stat;

typedef struct buffer_stat {
    void *buf;
    struct buffer_pool_stat *pool_Stat;
    SM_FileHandle *fh;
} buffer_stat;

typedef enum UpdateOptions {
    UP_Pin = 0,
    UP_Dirty = 1,
    UP_Unpin = 2,
    UP_NOT_Dirty = 3
} UpdateOptions;

#define MAKE_QUEUE_BUFFER()				\
  ((struct buffer_Queue *) malloc (sizeof(struct buffer_Queue)))

#define MAKE_BUFFER_STAT()				\
  ((buffer_stat *) malloc (sizeof(buffer_stat)))

#define MAKE_BUFFER_POOL_STAT()				\
  ((buffer_pool_stat *) malloc (sizeof(buffer_pool_stat)))

#define MAKE_LRU_BUFFER()					\
  ((struct buffer_LRU *) malloc (sizeof(struct buffer_LRU)))


/*
 * METHOD: Initialize the buffer pool
 * INPUT: File Name, Buffer Pool object, Number of pages in the pool and page replacement strategy used by the buffer pool
 * OUTPUT: RC_OK-SUCESSS;RC_OTHERS-ON FAIL;
 */
RC initBufferPool(BM_BufferPool * const bm, const char *const pageFileName, const int numPages, ReplacementStrategy strategy, void* stratData) {

    SM_FileHandle *fh = (SM_FileHandle *) malloc(PAGE_SIZE);
    memset(fh,0,PAGE_SIZE);
    buffer_stat *stat;
    buffer_pool_stat *pool_stat;
    PageNumber* pageNumArray;
    int* fixCountsArray;
    bool* dirtyMarked;
    int i = 0;
    stat = MAKE_BUFFER_STAT(); memset(stat,0,sizeof(buffer_stat));
    pool_stat = MAKE_BUFFER_POOL_STAT(); memset(pool_stat,0,sizeof(buffer_pool_stat));

    bm->pageFile = (char *) pageFileName;
    bm->numPages = numPages;
    bm->strategy = strategy;

    //initialize the pool_stat members
    pageNumArray = (PageNumber *) malloc(sizeof (PageNumber) * bm->numPages);memset(pageNumArray,0,sizeof (PageNumber) * bm->numPages);
    fixCountsArray = (int *) malloc(sizeof (int)*numPages);memset(fixCountsArray,0,sizeof (int)*numPages);
    dirtyMarked = (bool *) malloc(sizeof (bool) * numPages);memset(dirtyMarked,0,sizeof (bool) * numPages);
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

    //Open the Page File 
    openPageFile(bm->pageFile, fh);
    switch (strategy) {
        case RS_FIFO:
            stat->fh = fh;
            stat->buf = strt_fifo;
            stat->pool_Stat = pool_stat;
            break;
        case RS_LRU:
            stat->fh = fh;
            stat->buf = start_lru;
            stat->pool_Stat = pool_stat;
            break;
        case RS_CLOCK:
            //printf("CLOCK");
            break;
        case RS_LFU:
            //printf("LFU");
            break;
        case RS_LRU_K:
            //printf("LRU-K");
            break;
        default:
            return RC_BUFFER_UNDEFINED_STRATEGY;
    }
    bm->mgmtData = stat;
    //fh=(void*)realloc(fh,0);
    /*temp = (void *)realloc(temp,0);
    pool_stat->dirtyPages_stat = (void *)realloc(pool_stat->dirtyPages_stat,0);
    pool_stat->fixCounts_stat = (void *)realloc(pool_stat->fixCounts_stat,0);
    pool_stat->pageNumber_stat = (void *)realloc(pool_stat->pageNumber_stat,0);
    stat->fh = (void *)realloc( stat->fh, 0);
    stat->buf = (void *)realloc( stat->buf, 0);
    stat->pool_Stat = (void *)realloc( stat->pool_Stat, 0);
    stat = (void *)realloc(stat,0);*/
    return RC_OK;
}

int lengthofLRUBuffer() {
    struct buffer_LRU *temp;
    temp = start_lru;
    int count = 0;
    while (temp != NULL) {
        count++;
        temp = temp->next;
    }
    return count;
}

int searchForFrame_LRU(PageNumber pNum) {
    int pos = -1;
    struct buffer_LRU *temp;
    temp = start_lru;
    while (temp != NULL) {
        if (temp->pageNum == pNum) {
            pos = temp->pos;
            break;
        }
        temp = temp->next;
    }
    return pos;
}

int LRU() {
    struct buffer_LRU *temp;
    temp = start_lru;
    int pos;
    int max;
    max = temp->age; //Sets max to first value
    pos = temp->pos;
    while (temp != NULL) {
        if (temp->age > max) {
            max = temp->age;
            pos = temp->pos;
        }
        temp = temp->next;
    }
    return pos;
}

RC increment_age() {
    struct buffer_LRU *root;
    root = start_lru;
    while (root != NULL) {
        root->age = root->age + 1;
        root = root->next;
    }
    return RC_OK;
}

int maxposinLRU() {
    struct buffer_LRU *temp;
    temp = start_lru;
    int max;
    max = temp->pos; //Sets max to first value
    while (temp != NULL) {
        if (temp->pos > max) {
            max = temp->pos;
        }
        temp = temp->next;
    }
    return max;

}

/*
 * METHOD: Insert the page content in the buffer pool
 * INPUT: Buffer Pool object, Page Handle
 * OUTPUT: RC_OK-SUCESSS;RC_OTHERS-ON FAIL;
 */
RC insert_into_buffer_LRU(BM_BufferPool * const bm, BM_PageHandle * const page) {
    buffer_stat *stat;
    struct buffer_LRU *temp, *temp1;
    buffer_pool_stat *pool_stat;
    int searchRes;
    
    stat = bm->mgmtData;
    pool_stat = stat->pool_Stat;
    int sizeOfBuffer;
    int pos = 0;
    sizeOfBuffer = lengthofLRUBuffer();
    if (start_lru == NULL) {
        temp = MAKE_LRU_BUFFER();
        temp->pos = 0;
        temp->age = 1;
        temp->dirty = false;
        temp->fixCount = 1;
        temp->data = page->data;
        temp->pageNum = page->pageNum;
        start_lru = temp;
        start_lru->next = NULL;
        pool_stat->pageNumber_stat[0] = temp->pageNum;

    } else {

        searchRes = searchForFrame_LRU(page->pageNum);
        if (searchRes != -1) {
            //element found. page pos returned
            temp = start_lru;
            while (temp != NULL) {
                if (temp->pos == searchRes) {
                    break;
                }
                temp = temp->next;
            }
            increment_age();
            temp->age = 1;
            temp->dirty = false;
            temp->fixCount = temp->fixCount + 1;
            temp->data = page->data;
            pool_stat->pageNumber_stat[searchRes] = temp->pageNum;
        } else {
            //element not found
            if (sizeOfBuffer == bm->numPages) {
                pos = LRU();
                temp = start_lru;
                while (temp != NULL) {
                    if (temp->pos == pos) {
                        break;
                    }
                    temp = temp->next;
                }
                if (temp->dirty == true) {
                    writeBlock(temp->pageNum, stat->fh, temp->data);
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
            } else {
                temp1 = start_lru;
                while (temp1 != NULL) {
                    pos = temp1->pos;
                    temp1 = temp1->next;
                }
                pos = maxposinLRU();
                temp = MAKE_LRU_BUFFER();
                temp->pos = pos + 1;
                temp->dirty = false;
                temp->fixCount = 1;
                increment_age();
                temp->age = 1;
                temp->data = page->data;
                temp->pageNum = page->pageNum;
                temp->next = start_lru;
                start_lru = temp;
                pool_stat->pageNumber_stat[temp->pos] = temp->pageNum;
            }
        }
    }
    return RC_OK;

}


/*
 * METHOD: Shut down the buffer pool, write the dirty pages in the buffer pool to the disk
 * INPUT: Buffer Pool object
 * OUTPUT: RC_OK-SUCESSS;RC_OTHERS-ON FAIL;
 */
RC shutdownBufferPool(BM_BufferPool * const bm) {
    buffer_stat *stat;
    buffer_pool_stat *pool_stat;
    struct buffer_Queue *temp;
    struct buffer_LRU *temp1;

    stat = bm->mgmtData;
    pool_stat = stat->pool_Stat;

    switch (bm->strategy) {
        case RS_FIFO:
        {
            int sortedPageNum[bm->numPages];
            int j = 0;
            int i = 0;
            temp = strt_fifo;
            while (temp != NULL) {
                sortedPageNum[j] = temp->pageNum;
                temp = temp->next;
                j++;
            }

            
            for (i = 0; i < bm->numPages; i++) {
                for (j = 0; j < bm->numPages - 1; j++) {
                    if (sortedPageNum[j] > sortedPageNum[j + 1]) {
                        int temp1 = sortedPageNum[j];
                        sortedPageNum[j] = sortedPageNum[j + 1];
                        sortedPageNum[j + 1] = temp1;
                    }
                }
            }
            
            for(j = 0; j < bm->numPages; j++) {
                temp = strt_fifo;
                while (temp != NULL) {
                    if(sortedPageNum[j] == temp->pageNum){
                    if (temp->dirty == true) {
                        //debug statements: Naga
                        //printf("Naga: final page num %d and data is %s\n", temp->pageNum, temp->data);
                        writeBlock(temp->pageNum, stat->fh, temp->data);
                        pool_stat->writeIO = pool_stat->writeIO + 1;
                        temp->dirty = false;
                    }
                    
                }
                    temp = temp->next;
                }
            }           
            break;
        }
        case RS_LRU:
        {
            temp1 = start_lru;
            while (temp1 != NULL) {
                if (temp1->dirty) {
                    writeBlock(temp1->pageNum, stat->fh, temp1->data);
                    pool_stat->writeIO = pool_stat->writeIO + 1;
                }
                temp1 = temp1->next;
            }
            break;
        }
        case RS_CLOCK:
        {
            //printf("CLOCK");
            break;
        }
        case RS_LFU:
        {
            //printf("LFU");
            break;
        }
        case RS_LRU_K:
        {
            //printf("LRU-K");
            break;
        }
        default:
            return RC_BUFFER_UNDEFINED_STRATEGY;
    }

    strt_fifo = NULL;
    rear_fifo = NULL;
    handler = NULL;
    closePageFile(stat->fh);
    free(pool_stat->dirtyPages_stat);
    free(pool_stat->fixCounts_stat);
    free(pool_stat->pageNumber_stat);  
    free(stat->fh);
    free(stat->buf);
    free(stat->pool_Stat);
    free(bm->mgmtData);  
    
    //temp = (void *)realloc(temp,0);
    //pool_stat->dirtyPages_stat = (void *)realloc(pool_stat->dirtyPages_stat,0);
    //pool_stat->fixCounts_stat = (void *)realloc(pool_stat->fixCounts_stat,0);
    //pool_stat->pageNumber_stat = (void *)realloc(pool_stat->pageNumber_stat,0);
    //stat->fh = (void *)realloc( stat->fh, 0);
    //stat->buf = (void *)realloc( stat->buf, 0);
    //stat->pool_Stat = (void *)realloc( stat->pool_Stat, 0);
    //stat = (void *)realloc(stat,0);
    //bm->mgmtData = (void *)realloc(bm->mgmtData, 0);
       
    return RC_OK;
}


/*
 * METHOD: Force write all the dirty pages in the the buffer pool to disk
 * INPUT: Buffer Pool object
 * OUTPUT: RC_OK-SUCESSS;RC_OTHERS-ON FAIL;
 */
RC forceFlushPool(BM_BufferPool * const bm) {

    buffer_stat *stat;
    buffer_pool_stat *pool_stat;
    struct buffer_Queue *temp;
    struct buffer_LRU *temp1;

    stat = bm->mgmtData;
    pool_stat = stat->pool_Stat;

    switch (bm->strategy) {
        case RS_FIFO:
            temp = strt_fifo;
            while (temp != NULL) {
                if (temp->dirty == true) {
                    writeBlock(temp->pageNum, stat->fh, temp->data);
                    pool_stat->writeIO++;
                    temp->dirty = false;
                    pool_stat->dirtyPages_stat[temp->pos] = temp->dirty;
                }
                temp = temp->next;
            }
            break;
        case RS_LRU:
             temp1 = start_lru;
            while (temp1 != NULL) {
                if (temp1->dirty) {
                    writeBlock(temp1->pageNum, stat->fh, temp1->data);
                    pool_stat->writeIO = pool_stat->writeIO + 1;
                    temp1->dirty = false;
                }
                temp1 = temp1->next;
            }
            break;
            break;
        case RS_CLOCK:
            //printf("CLOCK");
            break;
        case RS_LFU:
            //printf("LFU");
            break;
        case RS_LRU_K:
            //printf("LRU-K");
            break;
        default:
            return RC_BUFFER_UNDEFINED_STRATEGY;
    }
    //temp = (void *)realloc(temp,0);
    /*pool_stat->dirtyPages_stat = (void *)realloc(pool_stat->dirtyPages_stat,0);
    pool_stat->fixCounts_stat = (void *)realloc(pool_stat->fixCounts_stat,0);
    pool_stat->pageNumber_stat = (void *)realloc(pool_stat->pageNumber_stat,0);
    stat->fh = (void *)realloc( stat->fh, 0);
    stat->buf = (void *)realloc( stat->buf, 0);
    stat->pool_Stat = (void *)realloc( stat->pool_Stat, 0);
    stat = (void *)realloc(stat,0);*/
    return RC_OK;
}


/*
 * METHOD: Search for a page number input in the FIFO buffer
 * INPUT: Page number
 * OUTPUT: page position in the FIFO buffer pool
 */
int search_frame_fifo_buffer(PageNumber pNum) {
    struct buffer_Queue *temp;
    int pos = -1;
    temp = strt_fifo;
    //debug statements : Naga
   // printf("Naga page Num passed to search fifo is %d\n", pNum);
    while (temp != NULL) {
        if (temp->pageNum == pNum) {
            pos = temp->pos;
            //debug statements : Naga
          //  printf("postion in the search function is %d\n", pos);
            break;
        }
        temp = temp->next;
    }
        //debug statements : Naga
    //printf("position returned in search function is %d\n", pos);
    return pos;
}


/*
 * METHOD: UPDATE the page feature (pin, unpin, mark dirty or mark un-dirty) in the FIFO buffer pool
 * INPUT: buffer related objects and update value
 * OUTPUT: RC_OK-SUCESSS;RC_OTHERS-ON FAIL;
 */
RC update_frame_stat_fifo_buffer(struct buffer_Queue *buf, buffer_stat *stat, UpdateOptions update_value, int pos) {
    buffer_pool_stat *pool_stat;
    pool_stat = stat->pool_Stat;
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


/*
 * METHOD: Mark the page dirty in the buffer pool
 * INPUT: Buffer pool object and page handle
 * OUTPUT: RC_OK-SUCESSS;RC_OTHERS-ON FAIL;
 */
RC markDirty(BM_BufferPool * const bm, BM_PageHandle * const page) {
    buffer_stat *stat;
    buffer_pool_stat *pool_stat;
    int pos = -1;
    struct buffer_LRU *node;

    stat = bm->mgmtData;
    pool_stat = stat->pool_Stat;

    switch (bm->strategy) {
        case RS_FIFO:
            //debug statements: Naga
          //  printf("page passed in mark dirty function %d\n", page->pageNum);
            pos = search_frame_fifo_buffer(page->pageNum);
            //debug statements : Naga
           // printf("Naga, position which is being dirty before call is %d", pos);
            //buf = strt_fifo;
            if (pos != -1) {
                update_frame_stat_fifo_buffer(strt_fifo, stat, UP_Dirty, pos);
                //debug statements : Naga
                //printf("Naga, position which is being after call dirty is %d\n" + pos);
            }
            break;
        case RS_LRU:
            pos = LRU(bm, page->pageNum);
            node = start_lru;
            while (node != NULL) {
                if (node->pos == pos) {
                    node->dirty = true;
                    pool_stat->dirtyPages_stat[pos] = node->dirty;
                }
                node = node->next;
            }
            break;
        case RS_CLOCK:
            //printf("CLOCK");
            break;
        case RS_LFU:
            //printf("LFU");
            break;
        case RS_LRU_K:
            //printf("LRU-K");
            break;
        default:
            return RC_BUFFER_UNDEFINED_STRATEGY;
    }

    /*pool_stat->dirtyPages_stat = (void *)realloc(pool_stat->dirtyPages_stat,0);
    pool_stat->fixCounts_stat = (void *)realloc(pool_stat->fixCounts_stat,0);
    pool_stat->pageNumber_stat = (void *)realloc(pool_stat->pageNumber_stat,0);
    stat->fh = (void *)realloc( stat->fh, 0);
    stat->buf = (void *)realloc( stat->buf, 0);
    stat->pool_Stat = (void *)realloc( stat->pool_Stat, 0);
    stat = (void *)realloc(stat,0);*/
    return RC_OK;
}

int len_fifo_buffer() {
    struct buffer_Queue *temp;
    int count = 0;
    temp = strt_fifo;
    while (temp != NULL) {
        count++;
        temp = temp->next;
    }
    return count;
}

RC insert_frame_fifo_buffer(BM_BufferPool * const bm, BM_PageHandle * const page) {
    int cur_buffer_size;
    int cur_free_frame=0;
    buffer_stat *stat;
    struct buffer_Queue *temp;
    buffer_pool_stat *pool_stat;
    
    stat = bm->mgmtData;
    pool_stat = stat->pool_Stat;
    if (strt_fifo == NULL && rear_fifo == NULL) {
        temp = MAKE_QUEUE_BUFFER(); memset(temp,0,sizeof(struct buffer_Queue));
        temp->pos = 0;
        temp->dirty = false;
        temp->fixCount = 1;
        temp->data=page->data;
        temp->pageNum = page->pageNum;
        rear_fifo = temp;
        strt_fifo = temp;
        strt_fifo->next = rear_fifo;
        rear_fifo->next = NULL;
        pool_stat->pageNumber_stat[temp->pos] = temp->pageNum;
        pool_stat->fixCounts_stat[temp->pos] = temp->fixCount;
        pool_stat->dirtyPages_stat[temp->pos] = temp->dirty;
        handler = strt_fifo;
        pool_stat->readIO++;
    } else {
        //if the page frame is full
        cur_buffer_size = len_fifo_buffer();
        if (bm->numPages == cur_buffer_size) {
            //temp1 = handler;
            while (cur_free_frame != bm->numPages) {
                if (handler->fixCount == 0) {
                    if (handler->dirty==true) {
                      
                        writeBlock(handler->pageNum, stat->fh, handler->data);
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
                        handler = strt_fifo;
                    } else {
                        handler = handler->next;
                    }
                    //temp1 = handler;
                    cur_free_frame++;
                }
            }
            if (cur_free_frame == bm->numPages) {
                return RC_BUFFER_EXCEEDED;
            } else {
                if (handler->next == NULL) {
                    handler = strt_fifo;
                } else {
                    handler = handler->next;
                }
            }
        }// else add the new page frame
        else {
            temp = MAKE_QUEUE_BUFFER(); memset(temp,0,sizeof(struct buffer_Queue));
            temp->next = NULL;
            temp->pos = rear_fifo->pos + 1;
            temp->dirty = false;
            temp->fixCount = 1;
            temp->data=page->data;
            temp->pageNum = page->pageNum;
            rear_fifo->next = temp;
            rear_fifo = temp;
            pool_stat->pageNumber_stat[temp->pos] = temp->pageNum;
            pool_stat->fixCounts_stat[temp->pos] = temp->fixCount;
            pool_stat->dirtyPages_stat[temp->pos] = temp->dirty;
            pool_stat->readIO++;
        }
    }
    /*temp = (void *)realloc(temp,0);
    temp1 = (void *)realloc(temp1,0);
    pool_stat->dirtyPages_stat = (void *)realloc(pool_stat->dirtyPages_stat,0);
    pool_stat->fixCounts_stat = (void *)realloc(pool_stat->fixCounts_stat,0);
    pool_stat->pageNumber_stat = (void *)realloc(pool_stat->pageNumber_stat,0);
    stat->fh = (void *)realloc( stat->fh, 0);
    stat->buf = (void *)realloc( stat->buf, 0);
    stat->pool_Stat = (void *)realloc( stat->pool_Stat, 0);
    stat = (void *)realloc(stat,0);*/
    return RC_OK;
}


/*
 * METHOD: Pin the page in the buffer pool
 * INPUT: Buffer pool object and page handle and the page num to be pinned
 * OUTPUT: RC_OK-SUCESSS;RC_OTHERS-ON FAIL;
 */
RC pinPage(BM_BufferPool * const bm, BM_PageHandle * const page, const PageNumber pageNum) {
    SM_PageHandle ph;
    buffer_stat *stat;
    buffer_pool_stat *pool_stat;
    int pos = -1;

    ph = (SM_PageHandle) malloc(PAGE_SIZE);
    memset(ph,0,PAGE_SIZE);

    stat = bm->mgmtData;
    pool_stat = stat->pool_Stat;

    page->pageNum = pageNum;
    switch (bm->strategy) {
        case RS_FIFO:
            pos = search_frame_fifo_buffer(page->pageNum);
            if(pos==-1){
            int rt = readBlock(pageNum, stat->fh, ph);
            //debug statements: Naga
                 //printf("Naga return code %i\n", rt);
            //debug statements : Naga
                 //printf("data read from file after pinning the page %s\n", ph);
             page->data=ph;          
            
            }
            //debug statements: Naga
//            printf("Naga, position to pin is at %d\n",pos);
            if (pos != -1) {
                update_frame_stat_fifo_buffer(strt_fifo, stat, UP_Pin, pos);
            } else {
                insert_frame_fifo_buffer(bm, page);
            }
            break;
        case RS_LRU:
            pos = searchForFrame_LRU(page->pageNum);
             if(pos == -1){
                readBlock(pageNum, stat->fh, ph);
                pool_stat->readIO = pool_stat->readIO+1;
                page->data=ph;
            }
            insert_into_buffer_LRU(bm,page);
            break;
        case RS_CLOCK:
            //printf("CLOCK");
            break;
        case RS_LFU:
            //printf("LFU");
            break;
        case RS_LRU_K:
            //printf("LRU-K");
            break;
        default:
            return RC_BUFFER_UNDEFINED_STRATEGY;
    }
    //ph = (void *)realloc(ph,0);
    /*pool_stat->dirtyPages_stat = (void *)realloc(pool_stat->dirtyPages_stat,0);
    pool_stat->fixCounts_stat = (void *)realloc(pool_stat->fixCounts_stat,0);
    pool_stat->pageNumber_stat = (void *)realloc(pool_stat->pageNumber_stat,0);
    stat->fh = (void *)realloc( stat->fh, 0);
    stat->buf = (void *)realloc( stat->buf, 0);
    stat->pool_Stat = (void *)realloc( stat->pool_Stat, 0);
    stat = (void *)realloc(stat,0);*/
    return RC_OK;
}

/*
 * METHOD: Unpin the page in the buffer pool
 * INPUT: Buffer pool object and page handle
 * OUTPUT: RC_OK-SUCESSS;RC_OTHERS-ON FAIL;
 */
RC unpinPage(BM_BufferPool * const bm, BM_PageHandle * const page) {
    buffer_stat *stat;
    struct buffer_Queue *buf;
    buffer_pool_stat *pool_stat;
    int pos = -1;

    struct buffer_LRU *bufLRU;
    stat = bm->mgmtData;
    pool_stat = stat->pool_Stat;
    switch (bm->strategy) {
        case RS_FIFO:
            pos = search_frame_fifo_buffer(page->pageNum);
            //debug statements: Naga
            //printf("Naga position used in unpin page block %d\n", pos);
            
            buf = strt_fifo;
            while (buf != NULL) {
                if (buf->pos == pos) {
                    //debug statements: Naga
                    //printf("Naga check page data before unpinning %s\n", page->data);
                    
                    update_frame_stat_fifo_buffer(strt_fifo, stat, UP_Unpin, pos);
                }
                buf = buf->next;
            }
            break;
        case RS_LRU:
            pos = searchForFrame_LRU(page->pageNum);
            bufLRU = start_lru;
            while (bufLRU != NULL) {
                if (bufLRU->pos == pos) {
                    bufLRU->fixCount = bufLRU->fixCount - 1;
                    pool_stat->fixCounts_stat[bufLRU->pos] = bufLRU->fixCount;
                }
                bufLRU = bufLRU->next;
            }
            break;
        case RS_CLOCK:
            //printf("CLOCK");
            break;
        case RS_LFU:
            //printf("LFU");
            break;
        case RS_LRU_K:
            //printf("LRU-K");
            break;
        default:
            return RC_BUFFER_UNDEFINED_STRATEGY;
    }
    /*buf = (void *)realloc(buf,0);
    pool_stat->dirtyPages_stat = (void *)realloc(pool_stat->dirtyPages_stat,0);
    pool_stat->fixCounts_stat = (void *)realloc(pool_stat->fixCounts_stat,0);
    pool_stat->pageNumber_stat = (void *)realloc(pool_stat->pageNumber_stat,0);
    stat->fh = (void *)realloc( stat->fh, 0);
    stat->buf = (void *)realloc( stat->buf, 0);
    stat->pool_Stat = (void *)realloc( stat->pool_Stat, 0);
    stat = (void *)realloc(stat,0);*/
    return RC_OK;
}


/*
 * METHOD: force write the page
 * INPUT: Buffer pool object and page handle
 * OUTPUT: RC_OK-SUCESSS;RC_OTHERS-ON FAIL;
 */
RC forcePage(BM_BufferPool * const bm, BM_PageHandle * const page) {
    buffer_stat *stat;
    buffer_pool_stat *pool_stat;
    struct buffer_Queue *temp;
    struct buffer_LRU *bufLRU;

    stat = bm->mgmtData;
    pool_stat = stat->pool_Stat;

    int pos = -1;

    switch (bm->strategy) {
        case RS_FIFO:
            pos = search_frame_fifo_buffer(page->pageNum);
            temp = strt_fifo;
            if (pos != -1) {
                while (temp != NULL) {
                    if (temp->pos == pos) {
                        temp->data=page->data;
                        //debug statements: Naga
                        //printf("Naga check page data before writing to the file %s\n", temp->data);
                        writeBlock(temp->pageNum, stat->fh, temp->data);
                        pool_stat->writeIO++;
                        update_frame_stat_fifo_buffer(stat->buf, stat, UP_NOT_Dirty, pos);
                    }
                    temp = temp->next;
                }
            }
            break;
        case RS_LRU:
            pos = searchForFrame_LRU(page->pageNum);
            bufLRU = start_lru;
            if (pos != -1) {
                while (bufLRU != NULL) {
                    if (bufLRU->pos == pos) {
                        //memcpy(bufLru->data, page->data, PAGE_SIZE);
                        bufLRU->data=page->data;
                        writeBlock(bufLRU->pageNum, stat->fh, bufLRU->data);
                        bufLRU->dirty = false;
                        pool_stat->dirtyPages_stat[pos] = bufLRU->dirty;
                        pool_stat->writeIO++;
                    }
                    bufLRU = bufLRU->next;
                }
            }
            break;
        case RS_CLOCK:
            //printf("CLOCK");
            break;
        case RS_LFU:
            //printf("LFU");
            break;
        case RS_LRU_K:
            //printf("LRU-K");
            break;
        default:
            return RC_BUFFER_UNDEFINED_STRATEGY;
    }
    /*temp = (void *)realloc(temp,0);
    pool_stat->dirtyPages_stat = (void *)realloc(pool_stat->dirtyPages_stat,0);
    pool_stat->fixCounts_stat = (void *)realloc(pool_stat->fixCounts_stat,0);
    pool_stat->pageNumber_stat = (void *)realloc(pool_stat->pageNumber_stat,0);
    stat->fh = (void *)realloc( stat->fh, 0);
    stat->buf = (void *)realloc( stat->buf, 0);
    stat->pool_Stat = (void *)realloc( stat->pool_Stat, 0);
    stat = (void *)realloc(stat,0);*/
    return RC_OK;
}


/*
 * METHOD: Get Frame Contents
 * INPUT: Buffer pool object
 * OUTPUT: Page number which is recently accessed
 */
PageNumber *getFrameContents(BM_BufferPool * const bm) {
    buffer_stat *stat;
    buffer_pool_stat *pool_stat;

    stat = bm->mgmtData;
    pool_stat = stat->pool_Stat;
    return pool_stat->pageNumber_stat;
}

/*
 * METHOD: Get Fix Counts
 * INPUT: Buffer pool object
 * OUTPUT: integer number which are pinned
 */
int *getFixCounts(BM_BufferPool * const bm) {
    buffer_stat *stat;
    buffer_pool_stat *pool_stat;

    stat = bm->mgmtData;
    pool_stat = stat->pool_Stat;

    return pool_stat->fixCounts_stat;

}

/*
 * METHOD: Get Presence of dirty pages in the buffer pool
 * INPUT: Buffer pool object
 * OUTPUT: boolean value
 */
bool *getDirtyFlags(BM_BufferPool * const bm) {
    buffer_stat *stat;
    buffer_pool_stat *pool_stat;

    stat = bm->mgmtData;
    pool_stat = stat->pool_Stat;

    return pool_stat->dirtyPages_stat;
}


/*
 * METHOD: Get Number of Reads from buffer pool
 * INPUT: Buffer pool object
 * OUTPUT: integer number 
 */
int getNumReadIO(BM_BufferPool * const bm) {
    buffer_stat *stat;
    buffer_pool_stat *pool_stat;
    stat = bm->mgmtData;
    pool_stat = stat->pool_Stat;

    return pool_stat->readIO;
}


/*
 * METHOD: Get Number of writes that happened in the buffer pool
 * INPUT: Buffer pool object
 * OUTPUT: integer number
 */
int getNumWriteIO(BM_BufferPool * const bm) {
    buffer_stat *stat;
    buffer_pool_stat *pool_stat;

    stat = bm->mgmtData;
    pool_stat = stat->pool_Stat;

    return pool_stat->writeIO;
}


