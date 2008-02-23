
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
int table_initialized = FALSE;

int load_match_file(off_t mode, char *fn) {

  off_t line_number = 0;
  char buf[MAX_STRING_LENGTH + 1];
  int file_type;
  FILE *f;

  /* We only need to initialize the table the first time through here.
     Otherwise, we'd erase all of the previous entries! */
  if (!table_initialized) 
  {
    hashTableInit(&knownHashes);
    table_initialized = TRUE;
  }

  if ((f = fopen(fn,"r")) == NULL) 
  {
    print_error(mode,fn,strerror(errno));
    return FALSE;
  }

  file_type = hash_file_type(f);
  if (file_type == TYPE_UNKNOWN)
  {
    print_error(mode,fn,"Unable to find hashes in file, skipped.");
    return FALSE;
  }

  /* We skip the first line in every file type except plain files. 
     All other file types have a header line that we need to ignore. */
  if (file_type == TYPE_PLAIN) 
    rewind(f);
  else 
    line_number++;
  
  while (fgets(buf,MAX_STRING_LENGTH,f)) {

    ++line_number;

    if (find_hash_in_line(buf,file_type) != TRUE) 
    {

      if (!(M_SILENT(mode)))
      {
	fprintf(stderr,"%s: %s: No hash found in line %llu\n", 
		__progname,fn,line_number);
      }

    } 
    else 
    {
      hashTableAdd(&knownHashes,buf);
    }
  }

  fclose(f);

#ifdef __DEBUG
  hashTableEvaluate(&knownHashes);
#endif

  return TRUE;
}



int is_known_hash(char *h) {
  return (hashTableContains(&knownHashes,h));
}

