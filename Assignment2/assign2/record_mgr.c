#include<string.h>
#include<stdlib.h>
#include<time.h>
#include "storage_mgr.h"
#include "buffer_mgr_stat.h"
#include "dberror.h"
#include "dt.h"
#include "tables.h"
#include "record_mgr.h"
#include "buffer_mgr.h"
#include<stdio.h>

#define DatabaseIntantiation "cs525Database"

typedef struct DatabaseContent
{
  char *name;
  void *mgmtData;
  int blk_Number;
  int blk_FreeMemory;
  int noOfblkRecords;
  int blkHeaderNumber;
  char *blkHeaderSchema;
  int blk_offset;
  
} DatabaseContent;

typedef struct ParseData {
    int currentRec;
    int numberRec;
    int noOfBlks;
    int currBlk;
    int parsedRecs;
    Expr *expression;
} ParseData;



 RC initRecordManager (void *mgmtData)
 {
	printf("Welcome to the Record Manager!! Initiated the record Manager.");
	return RC_OK;
 }

 RC shutdownRecordManager ()
 {
	printf("Record Manager shutting Down!! Swithced off the record Manager.");
	return RC_OK;
 }


 RC createTable (char *name, Schema *schema)
 {
	char *var = malloc(PAGE_SIZE);
	int fileDescriptor = open(name, O_RDWR);
        FILE *filePointer = fdopen(fileDescriptor,"wb");
	fseek(filePointer, 0L, SEEK_SET);
	memset(var, 0, PAGE_SIZE);
	fwrite(var,PAGE_SIZE,1, filePointer);
 
	return RC_OK;
 }

 RC openTable (RM_TableData *rel, char *name)
 {
	
	
 }
 RC closeTable (RM_TableData *rel){ return 0; }
 RC deleteTable (char *name){ return 0; }
 int getNumTuples (RM_TableData *rel){ return 0; }


 RC insertRecord (RM_TableData *rel, Record *record){ return 0; }
 RC deleteRecord (RM_TableData *rel, RID id){ return 0; }
 RC updateRecord (RM_TableData *rel, Record *record){ return 0; }
 RC getRecord (RM_TableData *rel, RID id, Record *record){ return 0; }


 RC startScan (RM_TableData *rel, RM_ScanHandle *scan, Expr *cond){ return 0; }
 RC next (RM_ScanHandle *scan, Record *record){ return 0; }
 RC closeScan (RM_ScanHandle *scan){ return 0; }


 int getRecordSize (Schema *schema){ return 0; }
/*
Create Schema Method.
*/
Schema *createSchema (int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys)
{ 
	Schema *obj = realloc(NULL, sizeof(Schema)); 
	obj->keySize = (int)keySize;
	obj->keyAttrs = (int*)keys;
	obj->typeLength = (int*) typeLength;
	obj->dataTypes =  (DataType*) dataTypes;
	obj->attrNames = (char**)attrNames;
	obj->numAttr = (int)numAttr;
	return obj;
}
 RC freeSchema (Schema *schema){ return 0; }


 RC createRecord (Record **record, Schema *schema){ return 0; }
 RC freeRecord (Record *record){ return 0; }
 RC getAttr (Record *record, Schema *schema, int attrNum, Value **value){ return 0; }
 RC setAttr (Record *record, Schema *schema, int attrNum, Value *value){ return 0; }
