// MD5DEEP - files.c
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
// $Id$

#include "main.h"
#include "common.h"

/* ---------------------------------------------------------------------
   How to add more file types that we can read known hashes from:

   1. Add a definition of TYPE_[fileType] to main.h. Ex: for type "cows",
      you would want to define TYPE_COWS.

   2. If your filetype has a rigid header, you should define a HEADER
      variable to make comparisons easier. 

   3. Add a check for your file type to the identify_hash_file_type() function.

   4. Create a method to find a valid hash and filename in a line of
      your file. You are encouraged to use find_plain_hash and
      find_rigid_hash if possible! 

   5. Add this method to the find_hash_in_line() function. 

   6. Add a variable to the state in main.h to indicate if each
      filetype supports this hash. The variable can be used to denote
      a position if necessary. Look for h_[name] variables for examples.

   7. Each hashing algorithm should set this variable in the 
      setup_hashing_algorithm function.
   
   ---------------------------------------------------------------------- */

typedef struct _ENCASE_HASH_HEADER {
  /* 000 */ char          Signature[8];
  /* 008 */ uint32_t      Version;
  /* 00c */ uint32_t      Padding;
  /* 010 */ uint32_t      NumHashes;
} ENCASE_HASH_HEADER;


#define ENCASE_HEADER    "HASH\x0d\x0a\xff\x00"

#define ILOOK_HEADER   \
"V1Hash,HashType,SetDescription,FileName,FilePath,FileSize"

#define ILOOK3_HEADER \
"V3Hash,HashSHA1,FileName,FilePath,FileSize,HashSHA256,HashSHA384,HashSHA512"

#define ILOOK4_HEADER \
"V4Hash,HashSHA1,FileName,FilePath,FileSize,HashSHA256,HashSHA384,HashSHA512,CreateTime,ModTime,LastAccessTime"

#define NSRL_15_HEADER    \
"\"SHA-1\",\"FileName\",\"FileSize\",\"ProductCode\",\"OpSystemCode\",\"MD4\",\"MD5\",\"CRC32\",\"SpecialCode\""

#define NSRL_20_HEADER \
"\"SHA-1\",\"MD5\",\"CRC32\",\"FileName\",\"FileSize\",\"ProductCode\",\"OpSystemCode\",\"SpecialCode\""

#define HASHKEEPER_HEADER \
"\"file_id\",\"hashset_id\",\"file_name\",\"directory\",\"hash\",\"file_size\",\"date_modified\",\"time_modified\",\"time_zone\",\"comments\",\"date_accessed\",\"time_accessed\""


#define HASH_STRING_LENGTH   (hashes[md5deep_mode_algorithm].bit_length/4)

/**
 * looks for a valid hash in the provided buffer.
 * returns TRUE if one is present.
 * @param known_fn - the hex hash is copied here.
 * @param buf      - input is the hash and the filename; the filename is left here.
 */
int state::find_plain_hash(char *buf, char *known_fn) 
{
    size_t p = HASH_STRING_LENGTH;
    
    if ((strlen(buf) < HASH_STRING_LENGTH) || (buf[HASH_STRING_LENGTH] != ' '))
	return FALSE;
    
    if (known_fn != NULL) {
	strncpy(known_fn,buf,PATH_MAX);
	
	// Starting at the end of the hash, find the start of the filename
	while(p < strlen(known_fn) && isspace(known_fn[p]))
	    ++p;
	shift_string(known_fn,0,p);
	chop_line(known_fn);
    }
    
    buf[HASH_STRING_LENGTH] = 0;

    /* We have to include a validity check here so that we don't
       mistake SHA-1 hashes for MD5 hashes, among other things */
    return algorithm_t::valid_hash(md5deep_mode_algorithm,buf);
}  

/**
 * detect the md5deep_size_hash file type, which has a size, a hash, and a filename.
 */
int state::find_md5deep_size_hash(char *buf, char *known_fn)
{
  size_t pos; 

  if (NULL == buf)
    return FALSE;

  // Extra 12 chars for size and space
  if (strlen(buf) < HASH_STRING_LENGTH + 12)
    return FALSE;
  
  // Check for size. Spaces are legal here (e.g. "      20")
  for (pos = 0 ; pos < 10 ; ++pos)
    if (!(isdigit(buf[pos]) || 0x20 == buf[pos]))
      return FALSE;
  
  if (buf[10] != 0x20 && buf[11] != 0x20)
    return FALSE;

  shift_string(buf,0,12);;

  return find_plain_hash(buf,known_fn);
}


int state::find_bsd_hash(char *buf, char *fn)
{
  char *temp;
  size_t buf_len = strlen(buf);
  unsigned int pos = 0, hash_len = HASH_STRING_LENGTH, 
    first_paren, second_paren;

  if (buf == NULL || buf_len < hash_len)
    return FALSE;

  while (pos < buf_len && buf[pos] != '(')
    ++pos;
  // The hash always comes after the file name, so there has to be 
  // enough room for the filename and *then* the hash.
  if (pos + hash_len + 1 > buf_len)
    return FALSE;
  first_paren = pos;

  // We only need to check back as far as the opening parenethsis,
  // not the start of the string. If the closing paren comes before
  // the opening paren (e.g. )( ) then the line is not valid
  pos = buf_len - hash_len;
  while (pos > first_paren && buf[pos] != ')')
    --pos;
  if (pos == first_paren)
    return FALSE;
  second_paren = pos;

  if (fn != NULL)
  {
    temp = strdup(buf);
    temp[second_paren] = 0;
    // The filename starts one character after the first paren
    shift_string(temp,0,first_paren+1);
    strncpy(fn,temp,PATH_MAX);
    free(temp);
  }

  // We chop instead of setting buf[HASH_STRING_LENGTH] = 0 just in
  // case there is extra data. We don't want to chop up longer 
  // (possibly invalid) data and take part of it as a valid hash!
  
  // We duplicate the buffer here as we're going to modify it.
  // We work on a copy so that we don't muck up the buffer for
  // any other functions who want to use it.
  temp = strdup(buf);
  chop_line(temp);

  // The hash always begins four characters after the second paren
  shift_string(temp,0,second_paren+4);

#if 0
  int status = valid_hash(s,temp);
  return status;
#endif
  assert(0);
  free(temp);

}
  

/* This is a generic function to find the filename and hash from a rigid
   (i.e. comma separated value) file format. Values may be quoted, but
   the quotes are removed before values are returned. The location variables
   refer to how many commas preceed the entry. For example, to get the
   hash out of:

   filename,junk,stuff,hash,stuff

   you should call find_rigid_hash(buf,fn,1,4);

   Note that columns start with #1, not zero. */
int state::find_rigid_hash(char *buf,  char *fn, unsigned int fn_location, unsigned int hash_location)
{
  char *temp = strdup(buf);
  if (temp == NULL)
    return FALSE;
  if (find_comma_separated_string(temp,fn_location-1))
  {
    free(temp);
    return FALSE;
  }
  strncpy(fn, temp, strlen(fn));
  free(temp);
  if (find_comma_separated_string(buf,hash_location-1))
    return FALSE;

  //return valid_hash(s,buf);
  assert(0);
}

#ifdef WORDS_BIGENDIAN
uint32_t byte_reverse(uint32_t n)
{
  uint32_t res = 0, count, bytes[5];
        
  for (count = 0 ; count < 4 ; count++)
    {
      bytes[count] = (n & (0xff << (count * 8))) >> (count * 8);
      res |= bytes[count] << (24 - (count * 8));
    }       
  return res;
}
#endif


/* iLook files have the MD5 hash as the first 32 characters. 
   As a result, we can just treat these files like plain hash files  */
int state::find_ilook_hash(char *buf, char *known_fn) 
{
  if (h_ilook)
    return (find_plain_hash(buf,known_fn));
  else
    return FALSE;
}


int state::check_for_encase(FILE *f,uint32_t *expected_hashes)
{
  ENCASE_HASH_HEADER *h = (ENCASE_HASH_HEADER *)malloc(sizeof(ENCASE_HASH_HEADER));
  
  if (NULL == h)
    fatal_error("Out of memory");
  
  if (sizeof(ENCASE_HASH_HEADER) != fread(h,1,sizeof(ENCASE_HASH_HEADER),f))  {
    free(h);
    return FALSE;
  }
  
  if (memcmp(h->Signature,ENCASE_HEADER,8))  {
    rewind(f);
    free(h);
    return FALSE;
  }           
  
#ifdef WORDS_BIGENDIAN
  h->NumHashes = byte_reverse(h->NumHashes);  
#endif

  *expected_hashes = h->NumHashes;
  return TRUE;
}


int state::identify_hash_file_type(FILE *f,uint32_t *expected_hashes) 
{
    char known_fn[PATH_MAX+1];
    char buf[MAX_STRING_LENGTH + 1];
    rewind(f);
    
    /* The "rigid" file types all have their headers in the 
       first line of the file. We check them first */
    
    if (h_encase)    {
	if (check_for_encase(f,expected_hashes))
	    return TYPE_ENCASE;
    }
    
    if ((fgets(buf,MAX_STRING_LENGTH,f)) == NULL) {
	return TYPE_UNKNOWN;
    }
    
    if (strlen(buf) > HASH_STRING_LENGTH) {

    chop_line(buf);

    if (h_hashkeeper)      {
	if (STRINGS_EQUAL(buf,HASHKEEPER_HEADER))
	  return TYPE_HASHKEEPER;
      }
    
    if (h_nsrl15)      {
	if (STRINGS_EQUAL(buf,NSRL_15_HEADER))
	  return TYPE_NSRL_15;
      }
    
    if (h_nsrl20) {
    if (STRINGS_EQUAL(buf,NSRL_20_HEADER))
      return TYPE_NSRL_20;
      }
    
    if (h_ilook)      {
	if (STRINGS_EQUAL(buf,ILOOK_HEADER))
	  return TYPE_ILOOK;
      }

    if (h_ilook3)      {
	if (STRINGS_EQUAL(buf,ILOOK3_HEADER))
	  return TYPE_ILOOK3;
      }

    if (h_ilook4)      {
	if (STRINGS_EQUAL(buf,ILOOK4_HEADER))
	  return TYPE_ILOOK3;
      }

    }
  
  
    /* Plain files can have comments, so the first line(s) may not
       contain a valid hash. But if we should process this file
       if we can find even *one* valid hash */
    do {
	if (find_bsd_hash(buf,known_fn)) {
	    return TYPE_BSD;
	}
	
	if (find_md5deep_size_hash(buf,known_fn)) {
	    return TYPE_MD5DEEP_SIZE;
	}
	
	if (find_plain_hash(buf,known_fn)) {
	    return TYPE_PLAIN;
	}
    } while ((fgets(buf,MAX_STRING_LENGTH,f)) != NULL);
    
    return TYPE_UNKNOWN;
}



/**
 * Given an input string buf and the type of file it came from, finds
 * the hash specified in the line if there is one and returns TRUE.
 * If there is no valid hash in the line, returns FALSE.  All
 * functions called from here are required to check that the hash is
 * valid before returning!
 */
int state::find_hash_in_line(char *buf, int fileType, char *fn) 
{
    switch(fileType) {
	
    case TYPE_PLAIN:	return find_plain_hash(buf,fn);
    case TYPE_BSD:	return find_bsd_hash(buf,fn);
    case TYPE_HASHKEEPER: return find_rigid_hash(buf,fn,3,h_hashkeeper);
    case TYPE_NSRL_15: return find_rigid_hash(buf,fn,2,h_nsrl15);
    case TYPE_NSRL_20: return find_rigid_hash(buf,fn,4,h_nsrl20);
    case TYPE_ILOOK:   return find_ilook_hash(buf,fn);
    case TYPE_ILOOK3:
	/* Intentional Fall Through */
    case TYPE_ILOOK4:
	return find_rigid_hash(buf,fn,3,h_ilook3);
    case TYPE_MD5DEEP_SIZE: return find_md5deep_size_hash(buf,fn);
    }
    return FALSE;
}


/**
 * explicitly add a hash that will be matched or not matched.
 * @param h - The hash (in hex)
 * @param fn - The file name (although -a and -A actually provide the hash again)
 */
void state::md5deep_add_hash(char *h, char *fn)
{
    class file_data_t *fdt = new file_data_t();
    fdt->hash_hex[md5deep_mode_algorithm] = h; 
    fdt->file_name = fn;
    known.add_fdt(fdt);
}



#if 0
/* from md5deep_match.cpp */

// Examines the hash table and determines if any known hashes have not
// been used or if any input files did not match the known hashes. If
// requested, displays any unused known hashes. Returns a status variable
int state::md5deep_finalize_matching()
{
  int status = STATUS_OK;

  if (input_not_matched)
    status |= STATUS_INPUT_DID_NOT_MATCH;

  if (hashTableDisplayNotMatched(&knownHashes, (s->mode & mode_not_matched)))
    status |= STATUS_UNUSED_HASHES;

  return status;
}
#endif


int input_not_matched = FALSE;

int file_type_without_header(int file_type)
{
  return (file_type == TYPE_PLAIN ||
	  file_type == TYPE_MD5DEEP_SIZE || 
	  file_type == TYPE_BSD);
}


#define ENCASE_START_HASHES 0x480
#define hash_length md5deep_mode_hash_length

int state::parse_encase_file(const char *fn, FILE *handle,uint32_t expected_hashes)
{
    unsigned char buffer[64];
    char result[1024];			// must be at least s->hash_length*2
    uint32_t count = 0;
  
    // Each hash entry is 18 bytes. 16 bytes for the hash and 
    // two \0 characters at the end. We reserve 19 characters 
    // as fread will append an extra \0 to the string 

    if (fseeko(handle,ENCASE_START_HASHES,SEEK_SET))  {
	print_error("%s: Unable to seek to start of hashes", fn);
	return STATUS_USER_ERROR;
    }
        
    while (!feof(handle)){		// 
	if (18 != fread(buffer,sizeof(unsigned char),18,handle)) {
	    if (feof(handle))
		continue;
        
	    // Users expect the line numbers to start at one, not zero.
	    if ((!opt_silent) || (mode & mode_warn_only)) {
		print_error("%s: No hash found in line %"PRIu32, fn, count + 1);
		print_error("%s: %s", fn, strerror(errno));
		return STATUS_USER_ERROR;
	    }
	}
	++count;        
                
	snprintf(result,sizeof(result),
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

	class file_data_t *fdt = new file_data_t();
	fdt->hash_hex[md5deep_mode_algorithm] = result; 
	fdt->file_name = fn;
	known.add_fdt(fdt);
    }

    if (expected_hashes != count){
	print_error(
		    "%s: Expecting %"PRIu32" hashes, found %"PRIu32"\n", 
		    fn, expected_hashes, count);
    }
    return STATUS_OK;
}



/**
 * Load an md5deep-style match file.
 * Previously this returned FALSE if failure and TRUE if success.
 * The return value was always ignored, so now we don't return anything.
 */
void state::md5deep_load_match_file(const char *fn) 
{
    uint64_t line_number = 0;
    char known_fn[PATH_MAX+1];
    int status;
    uint32_t expected_hashes=0;

    FILE *f= fopen(fn,"rb");
    if (f == NULL) {
	print_error("%s: %s", fn,strerror(errno));
	return;
    }

    int file_type = identify_hash_file_type(f,&expected_hashes);
    if (file_type == TYPE_UNKNOWN)  {
	print_error("%s: Unable to find any hashes in file, skipped.", fn);
	fclose(f);
	return;
    }

    if (TYPE_ENCASE == file_type)  {
	// We can't use the normal file reading code which is based on
	// a one-line-at-a-time approach. Encase files are binary records 
	status = parse_encase_file(fn,f,expected_hashes);
	fclose(f);
	return;
    }

    // We skip the first line in every file type except plain files. 
    // All other file types have a header line that we need to ignore.
    if (file_type_without_header(file_type)){
	rewind(f);
    }
    else {
	line_number++;
    }
  
    char buf[MAX_STRING_LENGTH + 1];
    while (fgets(buf,MAX_STRING_LENGTH,f)) {
	++line_number;
	memset(known_fn,0,PATH_MAX);

	if (!find_hash_in_line(buf,file_type,known_fn)) {
	    if ((!opt_silent) || (mode & mode_warn_only)) {
		fprintf(stderr,"%s: %s: No hash found in line %" PRIu64 "%s", 
			__progname,fn,line_number,NEWLINE);
	    }
	} else {
	    // Invalid hashes are caught above
	    file_data_t *fdt = new file_data_t();
	    fdt->hash_hex[md5deep_mode_algorithm] = buf; // the hex hash
	    fdt->file_name = known_fn;		    // the filename
	    known.add_fdt(fdt);
	}
    }
    fclose(f);
}



