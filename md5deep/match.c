// MD5DEEP - match.c
//
// By Jesse Kornblum
//
// This is a work of the US Government. In accordance with 17 USC 105,
// copyright protection is not available for any work of the US Government.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
//
// $Id$ 

#include "md5deep.h"
#include "hashTable.h"

hashTable knownHashes;
int table_initialized = FALSE;
int input_not_matched = FALSE;

int file_type_without_header(int file_type)
{
  return (file_type == TYPE_PLAIN ||
	  file_type == TYPE_MD5DEEP_SIZE || 
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
  
#define ENCASE_START_HASHES 0x480


static int parse_encase_file(state *s, char *fn, FILE *handle)
{
  unsigned char *buffer;
  char *result;
  uint32_t count = 0;
  
  if (NULL == s || NULL == fn || NULL == handle)
    return STATUS_INTERNAL_ERROR;
  
  // Each hash entry is 18 bytes. 16 bytes for the hash and 
  // two \0 characters at the end. We reserve 19 characters 
  // as fread will append an extra \0 to the string 
  MD5DEEP_ALLOC(unsigned char,buffer,19);
  MD5DEEP_ALLOC(char,result,(s->hash_length * 2) + 1);

  if (fseeko(handle,ENCASE_START_HASHES,SEEK_SET))
  {
    print_error(s,"%s: Unable to seek to start of hashes", fn);
    return STATUS_USER_ERROR;
  }
        
  while (!feof(handle))
  {
    if (18 != fread(buffer,sizeof(unsigned char),18,handle))
    {
      if (feof(handle))
	continue;
        
      // Users expect the line numbers to start at one, not zero.
      if ((!(s->mode & mode_silent)) || (s->mode & mode_warn_only))
      {
	print_error(s,"%s: No hash found in line %"PRIu32, fn, count + 1);
	print_error(s,"%s: %s", fn, strerror(errno));
	free(buffer);
	free(result);
	return STATUS_USER_ERROR;
      }
    }

    ++count;        
                
    snprintf(result,(s->hash_length * 2) + 1,
	 "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
	     buffer[0],
	     buffer[1],                      
	     buffer[2],
	     buffer[3],
	     buffer[4],                      
	     buffer[5],
	     buffer[6],
	     buffer[7],                              
	     buffer[8],                              
	     buffer[9],
	     buffer[10],                     
	     buffer[11],                             
	     buffer[12],                     
	     buffer[13],     
	     buffer[14],                     
	     buffer[15]);

    if (hashTableAdd(s,&knownHashes,result,fn))
    {
      print_error(s,"%s: %s: Out of memory at line %" PRIu32,
		  __progname, fn, count);
      return STATUS_INTERNAL_ERROR;
    }
  }

  free(buffer);
  free(result);
  
  if (s->expected_hashes != count)
    print_error(s,
		"%s: Expecting %"PRIu32" hashes, found %"PRIu32"\n", 
		fn, s->expected_hashes, count);

  return STATUS_OK;
}



int load_match_file(state *s, char *fn) 
{
  uint64_t line_number = 0;
  char buf[MAX_STRING_LENGTH + 1];
  char *known_fn;
  int file_type, status;
  FILE *f;

  if (NULL == s || NULL == fn)
    return TRUE;

  // We only need to initialize the table the first time through here.
  // Otherwise, we'd erase all of the previous entries!
  init_table();

  if ((f = fopen(fn,"rb")) == NULL) 
  {
    print_error(s,"%s: %s", fn,strerror(errno));
    return FALSE;
  }

  file_type = hash_file_type(s,f);
  if (file_type == TYPE_UNKNOWN)
  {
    print_error(s,"%s: Unable to find any hashes in file, skipped.", fn);
    fclose(f);
    return FALSE;
  }

  if (TYPE_ENCASE == file_type)
  {
    // We can't use the normal file reading code which is based on
    // a one-line-at-a-time approach. Encase files are binary records 
    status = parse_encase_file(s,fn,f);
    fclose(f);
    
    switch (status)
    {
    case STATUS_OK: 
    case STATUS_USER_ERROR:
      return TRUE;
    default: return FALSE;
    }
  }

  // We skip the first line in every file type except plain files. 
  // All other file types have a header line that we need to ignore.
  if (file_type_without_header(file_type))
    rewind(f);
  else 
    line_number++;
  
  if ((known_fn = (char *)malloc(sizeof(char) * PATH_MAX)) == NULL)
  {
    print_error(s,"%s: Out of memory before read", fn);
    return FALSE;
  }

  while (fgets(buf,MAX_STRING_LENGTH,f)) 
  {
    ++line_number;
    memset(known_fn,sizeof(char),PATH_MAX);

    if (find_hash_in_line(s,buf,file_type,known_fn) != TRUE) 
    {
      if ((!(s->mode & mode_silent)) || (s->mode & mode_warn_only))
      {
	fprintf(stderr,"%s: %s: No hash found in line %" PRIu64 "%s", 
		__progname,fn,line_number,NEWLINE);
      }
    } 
    else 
    {
      // Invalid hashes are caught above
      if (hashTableAdd(s,&knownHashes,buf,known_fn))
      {
	print_error(s,"%s: %s: Out of memory at line %" PRIu64 "%s",
		    __progname, fn, line_number, NEWLINE);
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



void add_hash(state *s, char *h, char *fn)
{
  if (NULL == s || NULL == h || NULL == fn)
    internal_error("%s: Null values passed into add_hash", __progname);

  init_table();
  switch (hashTableAdd(s,&knownHashes,h,fn))
  {
  case HASHTABLE_OK: break;
  case HASHTABLE_OUT_OF_MEMORY: 
    print_error(s,"%s: Out of memory", __progname); break;
  case HASHTABLE_INVALID_HASH: 
    print_error(s,"%s: Invalid hash", __progname); break;
  }
}


int is_known_hash(char *h, char *known_fn) 
{
  int status;

  // We don't check if the known_fn parameter is NULL because
  // that's a legitimate call in hash.c under mode_not_matched
  if (NULL == h)
    internal_error("%s: Null values passed into is_known_hash",__progname);

  if (!table_initialized)
    internal_error("%s: Attempt to check hash before table was initialized",
		   __progname);
  
  status = hashTableContains(&knownHashes,h,known_fn);
  if (!status)
    input_not_matched = TRUE;

  return status;
}


// Examines the hash table and determines if any known hashes have not
// been used or if any input files did not match the known hashes. If
// requested, displays any unused known hashes. Returns a status variable
int finalize_matching(state *s)
{
  int status = STATUS_OK;

  if (!table_initialized)
    internal_error("%s: Attempt to display unmatched hashes before table was initialized", __progname);
  
  if (input_not_matched)
    status |= STATUS_INPUT_DID_NOT_MATCH;

  if (hashTableDisplayNotMatched(&knownHashes, (s->mode & mode_not_matched)))
    status |= STATUS_UNUSED_HASHES;

  return status;
}

