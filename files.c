
/* MD5DEEP - files.c
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

/* ---------------------------------------------------------------------
   How to add more file types that we can read known hashes from:

   1. Add a definition of TYPE_[fileType] to md5deep.h. Ex: for type "cows",
      you would want to define TYPE_COWS.

   2. Create an is[fileType]File function. You may wish to use the 
      isSpecifiedFileType() function below. See the existing functions
      for examples. 

   3. Add your is[fileType]File function to the hash_file_type() function.

   4. Create a find[fileType]HashinLine function. See the existing functions
      for some good examples.

   5. Add your find[fileType]HashinLine function to the find_hash_in_line()
      function.

   6. Add your fileType to the findNameinLine function.

   7. Add a line to each hash algorithm header file (e.g. md5.h and sha1.h)
      to indicate which types of hashes the new file type contains.
   
   ---------------------------------------------------------------------- */


#define ILOOK_HEADER   \
"V1Hash,HashType,SetDescription,FileName,FilePath,FileSize"

#define NSRL_15_HEADER    \
"\"SHA-1\",\"FileName\",\"FileSize\",\"ProductCode\",\"OpSystemCode\",\"MD4\",\"MD5\",\"CRC32\",\"SpecialCode\""

#define NSRL_20_HEADER \
"\"SHA-1\",\"MD5\",\"CRC32\",\"FileName\",\"FileSize\",\"ProductCode\",\"OpSystemCode\",\"SpecialCode\""

#define HASHKEEPER_HEADER \
"\"file_id\",\"hashset_id\",\"file_name\",\"directory\",\"hash\",\"file_size\",\"date_modified\",\"time_modified\",\"time_zone\",\"comments\",\"date_accessed\",\"time_accessed\""



int valid_hash(char *buf) {
  int pos = 0;

  if (strlen(buf) < HASH_STRING_LENGTH) 
    return FALSE;

  for (pos = 0 ; pos < HASH_STRING_LENGTH ; pos++) 
    if (!isxdigit((int)buf[pos])) 
      return FALSE;
  return TRUE;
}


int isHashkeeperFile(char *buf) 
{
  return (!strncmp(buf,HASHKEEPER_HEADER,strlen(HASHKEEPER_HEADER)));
}

int isNSRL15File(char *buf) 
{
  return (!strncmp(buf,NSRL_15_HEADER,strlen(NSRL_15_HEADER)));
}

int isNSRL20File(char *buf) 
{
  return (!strncmp(buf,NSRL_20_HEADER,strlen(NSRL_20_HEADER)));
}


int isiLookFile(char *buf) {
  return (!strncmp(buf,ILOOK_HEADER,strlen(ILOOK_HEADER)));
}

int findPlainHashinLine(char *buf) {

  /* We have to include a validity check here so that we don't
     mistake SHA-1 hashes for MD5 hashes, among other things */

  if ((strlen(buf) < HASH_STRING_LENGTH) || 
      (buf[HASH_STRING_LENGTH] != ' '))
    return FALSE;

  buf[HASH_STRING_LENGTH] = 0;
  return (valid_hash(buf));
}  



/* Attempts to find a hash value after the nth quotation mark in buf */
int find_hash_after_quotes(char *buf, int n) {

  unsigned int count = 0, pos = 0;

  do {

    switch (buf[pos])
    {
    case '"': 
      count++;
      break;
    case 0:
      return FALSE;
    }   

    ++pos;

  } while (count < n);

  /* Check that there is enough left in the string to copy */
  if (pos + HASH_STRING_LENGTH > strlen(buf))
    return FALSE;

  /* Now we just copy the hash value back to the begining of the line */
  shift_string(buf,0,pos);
  buf[HASH_STRING_LENGTH] = 0;

  return TRUE;
}

int findHashkeeperHashinLine(char *buf) 
{
#ifdef SUPPORT_HASHKEEPER
  return (find_hash_after_quotes(buf,SUPPORT_HASHKEEPER));
#else
  return FALSE;
#endif
}

int findNSRL15HashinLine(char *buf) 
{
#ifdef SUPPORT_NSRL_15
  return (find_hash_after_quotes(buf,SUPPORT_NSRL_15));
#else
  return FALSE;
#endif
} 

int findNSRL20HashinLine(char *buf) 
{
#ifdef SUPPORT_NSRL_20
  return (find_hash_after_quotes(buf,SUPPORT_NSRL_20));
#else
  return FALSE
#endif
} 



/* iLook files have the MD5 hash as the first 32 characters. 
   As a result, we can just treat these files like plain hash files  */
int findiLookHashinLine(char *buf) 
{
#ifdef SUPPORT_ILOOK
  return (findPlainHashinLine(buf));
#else
  return FALSE;
#endif
}


int hash_file_type(FILE *f) {

  char buf[MAX_STRING_LENGTH + 1];
  rewind(f);

  /* The "rigid" file types all have their headers in the 
     first line of the file. We check them first */

  if ((fgets(buf,MAX_STRING_LENGTH,f)) == NULL) 
    return TYPE_UNKNOWN;
  
  if (strlen(buf) > HASH_STRING_LENGTH)
  {

#ifdef SUPPORT_HASHKEEPER
    if (isHashkeeperFile(buf)) 
      return TYPE_HASHKEEPER;
#endif
    
#ifdef SUPPORT_NSRL_15
    if (isNSRL15File(buf))
      return TYPE_NSRL_15;
#endif

#ifdef SUPPORT_NSRL_20
    if (isNSRL20File(buf))
      return TYPE_NSRL_20;
#endif

#ifdef SUPPORT_ILOOK
    if (isiLookFile(buf)) 
      return TYPE_ILOOK;
#endif
  }    

#ifdef SUPPORT_PLAIN
  /* Plain files can have comments, so the first line(s) may not
     contain a valid hash. But if we should process this file
     if we can find even *one* valid hash */
  do 
  {
    if (findPlainHashinLine(buf))
      return TYPE_PLAIN;
  } while ((fgets(buf,MAX_STRING_LENGTH,f)) != NULL);
#endif  

  return TYPE_UNKNOWN;
}



/* Given an input string buf and the type of file it came from, finds
   the MD5 hash specified in the line if there is one and returns TRUE.
   If there is no valid hash in the line, returns FALSE. */
int find_hash_in_line(char *buf, int fileType) {
  
  int status = FALSE;

  switch(fileType) {

    /* The findPlain already does a validity check for us */
  case TYPE_PLAIN:
    return findPlainHashinLine(buf);
    
  case TYPE_HASHKEEPER:
    status = findHashkeeperHashinLine(buf);
    break;
    
  case TYPE_NSRL_15:
    status = findNSRL15HashinLine(buf);
    break;

  case TYPE_NSRL_20:
    status = findNSRL20HashinLine(buf);
    break;
    
  case TYPE_ILOOK:
    status = findiLookHashinLine(buf);
    break;
  }

  if (status)
    return (valid_hash(buf));

  return FALSE;
}




