
/* MD5DEEP - match.c
 *
 * By Jesse Kornblum
 *
 * This is a work of the US Government. In accordance with 17 USC 105,
 * copyright protection is not available for any work of the US Government.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */


#include "md5deep.h"
#include "hashTable.h"

hashTable knownHashes;
bool tableInitialized = FALSE;

int loadMatchFile(char *filename) {

  unsigned long lineNumber = 0;
  char buf[MAX_STRING_LENGTH + 1];
  int fileType;
  FILE *f;

  /* We only need to initialize the table the first time through here.
     Otherwise, we'd erase all of the previous entries! */
  if (!tableInitialized) {
    hashTableInit(&knownHashes);
    tableInitialized = TRUE;
  }

  if ((f = fopen(filename,"r")) == NULL) {
    printError(filename);
    return FALSE;
  }

  fileType = determineFileType(f);
  
  /* We skip the first line in every file type except plain files. 
     All other file types have a header line that we need to ignore. */
  if (fileType != TYPE_PLAIN) 
    lineNumber++;
  else 
    rewind(f);
  
  while (fgets(buf,MAX_STRING_LENGTH,f)) {

    lineNumber++;

    if (findHashValueinLine(buf,fileType) != TRUE) {

      fprintf(stderr,"%s: %s: Improperly formatted file at line %ld\n",
	      __progname,filename,lineNumber);
      fprintf(stderr,"The offending line was:\n%s\n", buf);
      return FALSE;

    } else {

      hashTableAdd(&knownHashes,buf);

    }
  }

  fclose(f);

#ifdef __DEBUG
  hashTableEvaluate(&knownHashes);
#endif

  return TRUE;
}



int isKnownHash(char *h) {
  return (hashTableContains(&knownHashes,h));
}

