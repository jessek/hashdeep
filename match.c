
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
int input_not_matched = FALSE;

int file_type_without_header(int file_type)
{
  return (file_type == TYPE_PLAIN ||
	  file_type == TYPE_BSD);
}


void init_table(void)
{
  if (!table_initialized) 
  {
    hashTableInit(&knownHashes);
    table_initialized = TRUE;
  }
}
  


int load_match_file(uint64_t mode, char *fn) 
{
  uint64_t line_number = 0;
  char buf[MAX_STRING_LENGTH + 1];
  char *known_fn;
  int file_type;
  FILE *f;

  /* We only need to initialize the table the first time through here.
     Otherwise, we'd erase all of the previous entries! */
  init_table();

  if ((f = fopen(fn,"r")) == NULL) 
  {
    print_error(mode,fn,strerror(errno));
    return FALSE;
  }

  file_type = hash_file_type(f);
  if (file_type == TYPE_UNKNOWN)
  {
    print_error(mode,fn,"Unable to find any hashes in file, skipped.");
    return FALSE;
  }

  /* We skip the first line in every file type except plain files. 
     All other file types have a header line that we need to ignore. */
  if (file_type_without_header(file_type))
    rewind(f);
  else 
    line_number++;
  
  if ((known_fn = (char *)malloc(sizeof(char) * PATH_MAX)) == NULL)
  {
    print_error(mode,fn,"Out of memory before read");
    return FALSE;
  }

  while (fgets(buf,MAX_STRING_LENGTH,f)) 
  {
    ++line_number;
    memset(known_fn,sizeof(char),PATH_MAX);

    if (find_hash_in_line(buf,file_type,known_fn) != TRUE) 
    {
      if (!(M_SILENT(mode)))
      {
	fprintf(stderr,"%s: %s: No hash found in line %" PRIu64 "%s", 
		__progname,fn,line_number,NEWLINE);
      }

    } 
    else 
    {
      // Invalid hashes are caught above
      if (hashTableAdd(&knownHashes,buf,known_fn))
      {
	if (!(M_SILENT(mode)))
	{
	  fprintf(stderr,"%s: %s: Out of memory at line %" PRIu64 "%s",
		  __progname, fn, line_number, NEWLINE);
	}
	fclose(f);
	free(known_fn);
	return FALSE;
      }
    }
  }

  free(known_fn);
  fclose(f);

#ifdef __DEBUG
  hashTableEvaluate(&knownHashes);
#endif

  return TRUE;
}


void add_hash(uint64_t mode, char *h, char *fn)
{
  init_table();
  switch (hashTableAdd(&knownHashes,h,fn))
  {
  case HASHTABLE_OK: break;
  case HASHTABLE_OUT_OF_MEMORY: print_error(mode,h,"Out of memory"); break;
  case HASHTABLE_INVALID_HASH: print_error(mode,h,"Invalid hash"); break;
  }
}


int is_known_hash(char *h, char *known_fn) 
{
  int status;
  if (!table_initialized)
    internal_error("is_known_hash",
		   "attempted to check hash before table was initialized");
  
  status = hashTableContains(&knownHashes,h,known_fn);
  if (!status)
    input_not_matched = TRUE;

  return status;
}


/* Examines the hash table and determines if any known hashes have not
   been used or if any input files did not match the known hashes. If
   requested, displays any unused known hashes. Returns a status variable */   
int finalize_matching(uint64_t mode)
{
  int status = STATUS_OK;

  if (!table_initialized)
    internal_error("display_not_matched",
		   "attempted to display unmatched hashes before table was initialized");
  
  if (input_not_matched)
    status |= STATUS_INPUT_DID_NOT_MATCH;

  if (hashTableDisplayNotMatched(&knownHashes, M_NOT_MATCHED(mode)))
    status |= STATUS_UNUSED_HASHES;

  return status;
}

