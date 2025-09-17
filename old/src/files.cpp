/**
 * MD5DEEP - files.cpp
 *
 * By Jesse Kornblum
 * Substantially modified by Simson Garfinkel.
 *
 * This is a work of the US Government. In accordance with 17 USC 105,
 * copyright protection is not available for any work of the US Government.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Id$
 *
 * files.cpp was originally just part of the md5deep-family programs, not hashdeep.
 * It reads hash database files and incorporates them into the in-memory database.
 * Because it was designed for md5deep/sha1deep/etc, it would only look for a
 * single hash per line.
 *
 * The original program had variables named h_ilook, h_ilook3 and h_ilook4.
 * These variables were set to the variable number of the hash algorithm being looked for in each line.
 *
 */

#include "main.h"
#include "common.h"

#ifndef HAVE_ISXDIGIT
bool isxdigit(char ch)
{
    return isdigit(ch) || (ch >= 'a' && ch <= 'f') || (ch => 'A' && ch <= 'F');
}
#endif

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
   
   ----------------------------------------------------------------------
*/

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

#define HASH_STRING_LENGTH   (hashes[opt_md5deep_mode_algorithm].bit_length/4)

/****************************************************************
 *** Support Functions
 ****************************************************************/

// Find the index of the next comma in the string str starting at index start.
// quotes cause commas to be ingored until you are out of the quote.
// If there is no next comma, returns -1. 
static int find_next_comma(char *str, unsigned int start)
{
    assert(str);

    size_t size = strlen(str);
    unsigned int pos = start; 
    int in_quote = FALSE;
  
    while (pos < size)  {
	switch (str[pos]) {
	case '"':
	    in_quote = !in_quote;
	    break;
	case ',':
	    if (in_quote) break;

	    // Although it's potentially unwise to cast an unsigned int back
	    // to an int, problems will only occur when the value is beyond 
	    // the range of int. Because we're working with the index of a 
	    // string that is probably less than 32,000 characters, we should
	    // be okay. 
	    return (int)pos;
	}
	++pos;
    }
    return -1;
}

 

// Shift the contents of a string so that the values after 'new_start'
// will now begin at location 'start' 
void shift_string(char *fn, size_t start, size_t new_start)
{
    assert(fn!=0);

    // TODO: Can shift_string be replaced with memmove? 
    if (start > strlen(fn) || new_start < start) return;

    while (new_start < strlen(fn))    {
	fn[start] = fn[new_start];
	new_start++;
	start++;
    }

    fn[start] = 0;
}

// Returns the string after the nth comma in the string str. If that
// string is quoted, the quotes are removed. If there is no valid 
// string to be found, returns TRUE. Otherwise, returns FALSE 
static int find_comma_separated_string(char *str, unsigned int n)
{
    if (NULL == str) return TRUE;

    int start = 0, end;
    unsigned int count = 0; 
    while (count < n)  {
	if ((start = find_next_comma(str,start)) == -1) return TRUE;
	++count;
	// Advance the pointer past the current comma
	++start;
    }

    // It's okay if there is no next comma, it just means that this is
    // the last comma separated value in the string 
    if ((end = find_next_comma(str,start)) == -1)
	end = strlen(str);

    // Strip off the quotation marks, if necessary. We don't have to worry
    // about uneven quotation marks (i.e quotes at the start but not the end
    // as they are handled by the the find_next_comma function.
    if (str[start] == '"')
	++start;
    if (str[end - 1] == '"')
	end--;

    str[end] = 0;
    shift_string(str,0,start);
  
    return FALSE;
}





/**
 * looks for a valid hash in the provided buffer.
 * returns TRUE if one is present.
 * @param buf      - input is the hash and the filename; the hash is left here.
 * @param known_fn - the filename is copied there
 */
int state::find_plain_hash(char *buf, char *known_fn) 
{
    size_t p = HASH_STRING_LENGTH;
    
    if ((strlen(buf) < HASH_STRING_LENGTH) || (buf[HASH_STRING_LENGTH] != ' '))
	return FALSE;
    
    if (known_fn != NULL) {
	strncpy(known_fn,buf,PATH_MAX);
	
	// Starting at the end of the hash, find the start of the filename
	while(p < strlen(known_fn) && isspace(known_fn[p])){
	    ++p;
	}
	shift_string(known_fn,0,p);
	chop_line(known_fn);
    }
    
    buf[HASH_STRING_LENGTH] = 0;

    /* We have to include a validity check here so that we don't
       mistake SHA-1 hashes for MD5 hashes, among other things */
    return algorithm_t::valid_hash(opt_md5deep_mode_algorithm,buf);
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


/**
 * Look for a hash in a bsd-style buffer.
 * @param buf - input buffer; set to hash on output.
 * @param fn - gets filename
 *
 * bsd hash lines look like this:
 * MD5 (copying.txt) = 555b3e940c86b35d6e0c9976a05b3aa5
 */
int state::find_bsd_hash(char *buf, char *fn)
{
    size_t buf_len         = strlen(buf);
    unsigned int  hash_len = HASH_STRING_LENGTH;

    assert(buf!=0);
    if (buf_len < hash_len) return FALSE;

    char *open  = strchr(buf,'(');
    char *close = strchr(buf,')');
    char *equal = strchr(buf,'=');

    if(open==0 || close==0 || equal==0) return FALSE; // not properly formatted
    *close = '\000';			  // termiante the string
    strncpy(fn,open+1,PATH_MAX);
    
    /* Scan past the equal sign for the beginning of the hash */
    equal++;
    while(*equal!='\000' && !isxdigit(*equal)) equal++;

    /* Copy over buffer; */
    char *dest = buf;
    while(isxdigit(*equal)){
	*dest = *equal;
	dest++;
	equal++;
    }
    *dest = 0;				// terminate
    return algorithm_t::valid_hex(buf);
}
  

/* This is a generic function to find the filename and hash from a rigid
   (i.e. comma separated value) file format. Values may be quoted, but
   the quotes are removed before values are returned. The location variables
   refer to how many commas preceed the entry. For example, to get the
   hash out of:

   filename,junk,stuff,hash,stuff

   you should call find_rigid_hash(buf,fn,1,4);

   Note that columns start with #1, not zero.
*/

int state::find_rigid_hash(char *buf,  char *fn, unsigned int fn_location, unsigned int hash_location)
{
    char *temp = strdup(buf);
    if (temp == NULL)
	return FALSE;
    if (find_comma_separated_string(temp,fn_location-1))  {
	free(temp);
	return FALSE;
    }
    strncpy(fn, temp, strlen(fn));
    free(temp);
    if (find_comma_separated_string(buf,hash_location-1)){
	return FALSE;
    }

    return algorithm_t::valid_hash(opt_md5deep_mode_algorithm,buf);
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
    return (find_plain_hash(buf,known_fn));
}


int state::check_for_encase(FILE *f,uint32_t *expected_hashes)
{
  ENCASE_HASH_HEADER *h = (ENCASE_HASH_HEADER *)malloc(sizeof(ENCASE_HASH_HEADER));
  
  if (NULL == h) ocb.fatal_error("Out of memory");
  
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
     * first line of the file. We check them first
     */
    
    if (opt_md5deep_mode_algorithm == alg_md5 && check_for_encase(f,expected_hashes)) return TYPE_ENCASE;
    
    if ((fgets(buf,MAX_STRING_LENGTH,f)) == NULL) {
	return TYPE_UNKNOWN;
    }
    
    if (strlen(buf) > HASH_STRING_LENGTH) {
	
	chop_line(buf);

	/* Check for the algorithms that only have MD5 */
	if (opt_md5deep_mode_algorithm == alg_md5){
	    if(STRINGS_EQUAL(buf,HASHKEEPER_HEADER)) return TYPE_HASHKEEPER;
	    if (STRINGS_EQUAL(buf,ILOOK_HEADER)) return TYPE_ILOOK;
	}
	
	/* Check for those that have md5 or sha1 */
	if (opt_md5deep_mode_algorithm == alg_md5 || opt_md5deep_mode_algorithm == alg_sha1) {
	    if (STRINGS_EQUAL(buf,NSRL_15_HEADER)) return TYPE_NSRL_15;
	    if (STRINGS_EQUAL(buf,NSRL_20_HEADER)) return TYPE_NSRL_20;
	}
	
	/* Check for those that have md5 or sha1 or sha256 */
	if (opt_md5deep_mode_algorithm == alg_md5 || opt_md5deep_mode_algorithm == alg_sha1 || opt_md5deep_mode_algorithm == alg_sha256) {
	    if (STRINGS_EQUAL(buf,ILOOK3_HEADER)) return TYPE_ILOOK3;
	    if (STRINGS_EQUAL(buf,ILOOK4_HEADER)) return TYPE_ILOOK3;
	}
    }
  
  
    /* Plain files can have comments, so the first line(s) may not
     * contain a valid hash. But if we should process this file
     * if we can find even *one* valid hash
     */
    do {
	if (find_bsd_hash(buf,known_fn)) return TYPE_BSD;
	if (find_md5deep_size_hash(buf,known_fn)) return TYPE_MD5DEEP_SIZE;
	if (find_plain_hash(buf,known_fn)) return TYPE_PLAIN;
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
    case TYPE_PLAIN:	    return find_plain_hash(buf,fn);
    case TYPE_BSD:	    return find_bsd_hash(buf,fn);
    case TYPE_HASHKEEPER:   return find_rigid_hash(buf,fn,3,h_hashkeeper);
    case TYPE_NSRL_15:
	if(opt_md5deep_mode_algorithm == alg_md5) return find_rigid_hash(buf,fn,2,7);
	if(opt_md5deep_mode_algorithm == alg_sha1) return find_rigid_hash(buf,fn,2,1);
	return FALSE;			// NSRL_15 only hash md5 and sha1
    case TYPE_NSRL_20:
	if(opt_md5deep_mode_algorithm == alg_md5) return find_rigid_hash(buf,fn,4,2);
	if(opt_md5deep_mode_algorithm == alg_sha1) return find_rigid_hash(buf,fn,4,1);
	return FALSE;
    case TYPE_ILOOK:        return find_ilook_hash(buf,fn);
    case TYPE_ILOOK3:
	if(opt_md5deep_mode_algorithm == alg_md5)    return find_rigid_hash(buf,fn,3,1);
	if(opt_md5deep_mode_algorithm == alg_sha1)   return find_rigid_hash(buf,fn,3,2);
	if(opt_md5deep_mode_algorithm == alg_sha256) return find_rigid_hash(buf,fn,3,6);
	return FALSE;			// ilook3 only has md5, sha1 and sha256
    case TYPE_ILOOK4:	    
	if(opt_md5deep_mode_algorithm == alg_md5)    return find_rigid_hash(buf,fn,3,1);
	if(opt_md5deep_mode_algorithm == alg_sha1)   return find_rigid_hash(buf,fn,3,2);
	if(opt_md5deep_mode_algorithm == alg_sha256) return find_rigid_hash(buf,fn,3,6);
	return FALSE;			// ilook4 only has md5, sha1 and sha256
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
    fdt->hash_hex[opt_md5deep_mode_algorithm] = h; 
    fdt->file_name = fn;
    ocb.add_fdt(fdt);
}



/* from md5deep_match.cpp */



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
	ocb.error("%s: Unable to seek to start of hashes", fn);
	return status_t::STATUS_USER_ERROR;
    }
        
    while (!feof(handle)){		// 
	if (18 != fread(buffer,sizeof(unsigned char),18,handle)) {
	    if (feof(handle))
		continue;
        
	    // Users expect the line numbers to start at one, not zero.
	    if ((!ocb.opt_silent) || (mode_warn_only)) {
		ocb.error("%s: No hash found in line %"PRIu32, fn, count + 1);
		ocb.error("%s: %s", fn, strerror(errno));
		return status_t::STATUS_USER_ERROR;
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
	fdt->hash_hex[opt_md5deep_mode_algorithm] = result; 
	fdt->file_name = fn;
	ocb.add_fdt(fdt);
    }

    if (expected_hashes != count){
	ocb.error("%s: Expecting %"PRIu32" hashes, found %"PRIu32"\n", 
			fn, expected_hashes, count);
    }
    return status_t::status_ok;
}



/**
 * Load an md5deep-style match file.
 * Previously this returned FALSE if failure and TRUE if success.
 * The return value was always ignored, so now we don't return anything.
 */
void state::md5deep_load_match_file(const char *fn) 
{
    uint64_t line_number = 0;
    uint32_t expected_hashes=0;

    FILE *f= fopen(fn,"rb");
    if (f == NULL) {
	ocb.error("%s: %s", fn,strerror(errno));
	return;
    }

    int ftype = identify_hash_file_type(f,&expected_hashes);
    if (ftype == TYPE_UNKNOWN)  {
	ocb.error("%s: Unable to find any hashes in file, skipped.", fn);
	fclose(f); f = 0;
	return;
    }

    if (TYPE_ENCASE == ftype)  {
	// We can't use the normal file reading code which is based on
	// a one-line-at-a-time approach. Encase files are binary records 
        parse_encase_file(fn,f,expected_hashes);
	fclose(f); f = 0;
	return;
    }

    // We skip the first line in every file type except plain files. 
    // All other file types have a header line that we need to ignore.
    if (file_type_without_header(ftype)){
	rewind(f);
    }
    else {
	++line_number;
    }
  
    char buf[MAX_STRING_LENGTH + 1];
    while (fgets(buf,MAX_STRING_LENGTH,f)) {
	char *cc;
	char known_fn[PATH_MAX+1];		     // set to be the filename from the buffer
	if((cc=strchr(buf,'\n'))!=0) *cc = 0;	     // remove \n at end of line
	if((cc=strchr(buf,'\r'))!=0) *cc = 0;	     // remove \r at end of line
	++line_number;
	memset(known_fn,0,PATH_MAX);

	/* This looks odd. The function find_hash_in_line modifies 'buf' so that it
	 * begins with the hash, and copies the filename to known_fn.
	 */
	if (!find_hash_in_line(buf,ftype,known_fn)) {
	    if ((!ocb.opt_silent) || (mode_warn_only)) {
		std::cerr << progname << ": " << fn << ": No hash found in line " << line_number << std::endl;
	    }
	} else {
	    // Invalid hashes are caught above
	    file_data_t *fdt = new file_data_t();
	    fdt->hash_hex[opt_md5deep_mode_algorithm] = buf; // the hex hash
	    fdt->file_name = known_fn;		    // the filename
	    ocb.add_fdt(fdt);
	}
    }
    fclose(f);
    f = 0;
}



