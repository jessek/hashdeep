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

/* ---------------------------------------------------------------------
   How to add more file types that we can read known hashes from:

   1. Add a definition of TYPE_[fileType] to main.h. Ex: for type "cows",
      you would want to define TYPE_COWS.

   2. If your filetype has a rigid header, you should define a HEADER
      variable to make comparisons easier. 

   3. Add a check for your file type to the hash_file_type() function.

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




int valid_hash(state *s, char *buf) 
{
  size_t pos = 0;

  if (strlen(buf) < HASH_STRING_LENGTH) 
    return FALSE;

  for (pos = 0 ; pos < HASH_STRING_LENGTH ; pos++) 
    if (!isxdigit(buf[pos]))
      return FALSE;

  return TRUE;
}


int find_plain_hash(state *s, char *buf, char *known_fn) 
{
  size_t p = HASH_STRING_LENGTH;

  if (buf == NULL)
    return FALSE;

  if (strlen(buf) < HASH_STRING_LENGTH)
    return FALSE;
  
  // Is there any file information here?
  if (strlen(buf) == HASH_STRING_LENGTH)
  {
    // No. Copy default filename
    snprintf(known_fn,PATH_MAX,"(unknown)");
  }
  else
  {
    // Yes, there is something which should be a filename. Validate it
    // and copy it if necessary
    if (buf[HASH_STRING_LENGTH] != ' ')
      return FALSE;

    if (known_fn != NULL)
    {
      strncpy(known_fn,buf,PATH_MAX);

      // Starting at the end of the hash, find the start of the filename
      while(p < strlen(known_fn) && isspace(known_fn[p]))
	++p;
      shift_string(known_fn,0,p);
      chop_line(known_fn);
    }

    buf[HASH_STRING_LENGTH] = 0;
  }

  // We have to include a validity check here so that we don't
  // mistake SHA-1 hashes for MD5 hashes, among other things 
  return (valid_hash(s,buf));
}  

int find_md5deep_size_hash(state *s, char *buf, char *known_fn)
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

  return find_plain_hash(s,buf,known_fn);
}


int find_bsd_hash(state *s, char *buf, char *fn)
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

  int status = valid_hash(s,temp);

  // Okay, this is a valid hash. Go ahead and modify the original buffer
  // The extra character is for the terminating null.
  strncpy(buf,temp,HASH_STRING_LENGTH + 1);
  // We used to run chop_line() on buf, but for some reason buf was not
  // being NULL terminated. We force an explicit termination. -jdk 20 Dec 2011
  buf[HASH_STRING_LENGTH] = 0;

  free(temp);

  return status;
}
  

/* This is a generic function to find the filename and hash from a rigid
   (i.e. comma separated value) file format. Values may be quoted, but
   the quotes are removed before values are returned. The location variables
   refer to how many commas preceed the entry. For example, to get the
   hash out of:

   filename,junk,stuff,hash,stuff

   you should call find_rigid_hash(buf,fn,1,4);

   Note that columns start with #1, not zero. */
int find_rigid_hash(state *s, char *buf, char *fn, 
		      unsigned int fn_location, 
		      unsigned int hash_location)
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

  return valid_hash(s,buf);
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
int find_ilook_hash(state *s, char *buf, char *known_fn) 
{
  if (s->h_ilook)
    return (find_plain_hash(s,buf,known_fn));
  else
    return FALSE;
}


static int check_for_encase(state *s, FILE *f)
{
  ENCASE_HASH_HEADER *h = (ENCASE_HASH_HEADER *)malloc(sizeof(ENCASE_HASH_HEADER));
  
  if (NULL == h)
    fatal_error(s,"Out of memory");
  
  if (sizeof(ENCASE_HASH_HEADER) != fread(h,1,sizeof(ENCASE_HASH_HEADER),f))
  {
    free(h);
    return FALSE;
  }
  
  if (memcmp(h->Signature,ENCASE_HEADER,8))
  {
    rewind(f);
    free(h);
    return FALSE;
  }           
  
#ifdef WORDS_BIGENDIAN
  h->NumHashes = byte_reverse(h->NumHashes);  
#endif

  s->expected_hashes = h->NumHashes;
  return TRUE;
}


int hash_file_type(state *s, FILE *f) 
{
  char *known_fn;
  char buf[MAX_STRING_LENGTH + 1];
  rewind(f);

  /* The "rigid" file types all have their headers in the 
     first line of the file. We check them first */

  if (s->h_encase)
    {
      if (check_for_encase(s,f))
	return TYPE_ENCASE;
    }

  if ((fgets(buf,MAX_STRING_LENGTH,f)) == NULL) 
    return TYPE_UNKNOWN;
  
  if (strlen(buf) > HASH_STRING_LENGTH)
  {
    chop_line(buf);

    if (s->h_hashkeeper)
      {
	if (STRINGS_EQUAL(buf,HASHKEEPER_HEADER))
	  return TYPE_HASHKEEPER;
      }
    
    if (s->h_nsrl15)
      {
	if (STRINGS_EQUAL(buf,NSRL_15_HEADER))
	  return TYPE_NSRL_15;
      }
    
    if (s->h_nsrl20)
      {
    if (STRINGS_EQUAL(buf,NSRL_20_HEADER))
      return TYPE_NSRL_20;
      }
    
    if (s->h_ilook)
      {
	if (STRINGS_EQUAL(buf,ILOOK_HEADER))
	  return TYPE_ILOOK;
      }

    if (s->h_ilook3)
      {
	if (STRINGS_EQUAL(buf,ILOOK3_HEADER))
	  return TYPE_ILOOK3;
      }

    if (s->h_ilook4)
      {
	if (STRINGS_EQUAL(buf,ILOOK4_HEADER))
	  return TYPE_ILOOK3;
      }

  }
  
  
  /* Plain files can have comments, so the first line(s) may not
     contain a valid hash. But if we should process this file
     if we can find even *one* valid hash */
  known_fn = (char *)malloc(sizeof(char) * PATH_MAX);
  do 
  {
    if (find_bsd_hash(s,buf,known_fn))
    {
      free(known_fn);
      return TYPE_BSD;
    }

    if (find_md5deep_size_hash(s,buf,known_fn))
    {
      free(known_fn);
      return TYPE_MD5DEEP_SIZE;
    }

    if (find_plain_hash(s,buf,known_fn))
    {
      free(known_fn);
      return TYPE_PLAIN;
    }
  } while ((fgets(buf,MAX_STRING_LENGTH,f)) != NULL);
  free(known_fn);

  return TYPE_UNKNOWN;
}



/* Given an input string buf and the type of file it came from, finds
   the hash specified in the line if there is one and returns TRUE.
   If there is no valid hash in the line, returns FALSE. 
   All functions called from here are required to check that the hash
   is valid before returning! */
int find_hash_in_line(state *s, char *buf, int fileType, char *fn) 
{
  chop_line(buf);

  switch(fileType) {

  case TYPE_PLAIN:
    return find_plain_hash(s,buf,fn);

  case TYPE_BSD:
    return find_bsd_hash(s,buf,fn);

  case TYPE_HASHKEEPER:
    return (find_rigid_hash(s,buf,fn,3,s->h_hashkeeper));

  case TYPE_NSRL_15:
    return (find_rigid_hash(s,buf,fn,2,s->h_nsrl15));
    break;

  case TYPE_NSRL_20:
    return (find_rigid_hash(s,buf,fn,4,s->h_nsrl20));

  case TYPE_ILOOK:
    return (find_ilook_hash(s,buf,fn));

  case TYPE_ILOOK3:
  case TYPE_ILOOK4:
    return (find_rigid_hash(s,buf,fn,3,s->h_ilook3));

  case TYPE_MD5DEEP_SIZE:
    return (find_md5deep_size_hash(s,buf,fn));

	  
  }

  return FALSE;
}
