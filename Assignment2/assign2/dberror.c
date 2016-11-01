#include "dberror.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

char *RC_message = NULL;

/* print a message to standard out describing the error */
void 
printError (RC error)
{
  if (RC_message != NULL)
    printf("EC (%i), \"%s\"\n", error, RC_message);
  else
    printf("EC (%i)\n", error);
}

char* ErrorMessage(RC error)
{
	switch(error)
	{
		case RC_FILE_NOT_FOUND: 
			RC_message = "File Not Found in the directory provided.";
			break;
		case RC_FILE_HANDLE_EMPTY:
			RC_message = "File Handle Object not initialized yet.";
			break;
		case RC_WRITE_FAILED:
			RC_message = "Writing block into file failed.";
			break;
		case RC_READ_NON_EXISTING_PAGE:
			RC_message = "Non Existing Page cannot be read.";
			break;
		case RC_FILE_NOT_OPENED:
			RC_message = "File not Opened.";
			break;
		case RC_FILE_NOT_CLOSED:
			RC_message = "File not Closed.";
			break;	
		case RC_FILE_BUSY:
			RC_message = "File not Closed.";
			break;	
		case RC_FILE_NOT_DELETED:
			RC_message = "File not Deleted.";
			break;	
		case RC_ENOMEM:
			RC_message = "No memory available to allocate.";
			break;	
		case RC_FILE_NAME_EMPTY:
			RC_message = "File Name is empty.";
			break;	
		case RC_FILE_NOT_CREATED:
			RC_message = "File creation error.";
			break;

			
		default: break;
	}
	return NULL;
}

char *
errorMessage (RC error)
{
  char *message;
  ErrorMessage(error);
  if (RC_message != NULL)
    {
      message = (char *) malloc(strlen(RC_message) + 30);
      sprintf(message, "EC (%i), \"%s\"\n", error, RC_message);
    }
  else
    {
      message = (char *) malloc(30);
      sprintf(message, "EC (%i)\n", error);
    }

  return message;
}


