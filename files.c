
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


/* How to make this code able to handle more file types:

   1. Add a definition of TYPE_[fileType] to md5deep.h. Ex: for type "cows",
      you would want to define TYPE_COWS.

   2. Create an is[fileType]File function. You may wish to use the 
      isSpecifiedFileType() function below. See the existing functions
      for examples. 

   3. Add your is[fileType]File function to the determineFileType() function.

   4. Create a find[fileType]HashinLine function. See the existing functions
      for some good examples.

   5. Add your find[fileType]HashinLine function to the findHashinLine()
      function.

   6. Add your fileType to the findNameinLine function.
*/


#define ILOOK_HEADER   \
"V1Hash,HashType,SetDescription,FileName,FilePath,FileSize"

#define NSRL_HEADER    \
"\"SHA-1\",\"FileName\",\"FileSize\",\"ProductCode\",\"OpSystemCode\",\"MD4\",\"MD5\",\"CRC32\",\"SpecialCode\""

#define HASHKEEPER_HEADER \
"\"file_id\",\"hashset_id\",\"file_name\",\"directory\",\"hash\",\"file_size\",\"date_modified\",\"time_modified\",\"time_zone\",\"comments\",\"date_accessed\",\"time_accessed\""



bool isValidHash(char *buf) {
  int pos = 0;

  if (strlen(buf) < HASH_STRING_LENGTH) 
    return FALSE;

  for (pos = 0 ; pos < HASH_STRING_LENGTH ; pos++) 
    if (!isxdigit((int)buf[pos])) 
      return FALSE;
  return TRUE;
}


/* A plain hash file has 32 hash characters at the start of each line
   followed by at least one space. */
bool isPlainFile(char *buf) {

  if (!isValidHash(buf))
    return FALSE;

  return (buf[HASH_STRING_LENGTH] == 32);
}


bool isHashkeeperFile(char *buf) {
  return (!strncmp(buf,HASHKEEPER_HEADER,strlen(HASHKEEPER_HEADER)));
}

bool isNSRLFile(char *buf) {
  return (!strncmp(buf,NSRL_HEADER,strlen(NSRL_HEADER)));
}

bool isiLookFile(char *buf) {
  return (!strncmp(buf,ILOOK_HEADER,strlen(ILOOK_HEADER)));
}




int determineFileType(FILE *f) {

  char buf[MAX_STRING_LENGTH + 1];
  rewind(f);

  if ((fgets(buf,MAX_STRING_LENGTH,f)) == NULL) 
    return FALSE;

  if (isHashkeeperFile(buf)) 
    return TYPE_HASHKEEPER;

  if (isNSRLFile(buf))
    return TYPE_NSRL;

  if (isiLookFile(buf)) 
    return TYPE_ILOOK;

  /* We always look for plain files last. Other types of files can
     be mistaken for plain files.  */
  if (isPlainFile(buf))
    return TYPE_PLAIN;

  return TYPE_UNKNOWN;
}



/* Given an input string buf and the type of file it came from,
   finds the filename associated with the given hash value.
   This function was originally written for another application and
   is not used in md5deep. Therefore, it's okay that it's incomplete here.
   When the other program is developed, this function will be 
   fully implemented. */
#ifdef __SANTA
bool findNameinLine(char *buf, int fileType, char *name) {

  int pos;

  switch (fileType) {

  case TYPE_PLAIN:

    for (pos = HASH_STRING_LENGTH + 2 ; buf[pos] != 0 ; pos++) {
      name[pos - (HASH_STRING_LENGTH + 2)] = buf[pos];
    }
    name[pos - (HASH_STRING_LENGTH + 2)] = 0;
    return TRUE;

  /* RBF - Write findNameinLine */  
  case TYPE_HASHKEEPER:
  case TYPE_ILOOK:
  case TYPE_NSRL:
  default:
    strncpy(name,"Unknown name",15);
    return TRUE;
  }

  return TRUE;
}
#endif   /* #ifdef __SANTA */


bool findPlainHashinLine(char *buf) {

 /* We don't have to check the string length here.
     It's done inside isValidHash */
  if (!isValidHash(buf))
    return FALSE;
  
  buf[HASH_STRING_LENGTH] = 0;
  return TRUE;
}  



/* Attempts to find a hash value after the nth quotation mark in buf */
bool findHashAfterQuotes(char *buf, int n) {

  int count = 0, pos = -1;

  while (count < n) {

    pos++;
    if (buf[pos] == '"') {
      count++;

      /* We don't *need* to continue here, but by skipping the if
	 statement that comes next, we should be faster.
	 Yes, I'm that concerned about speed. */
      continue;
    }

    if (buf[pos] == 0) 
      return FALSE;
  }

  /* The current character is still the leading quote to the hash.
     We advance one to get to the real start of the hash */
  pos++;
  
  /* Now we just copy the hash value back to the begining of the line
     and re-terminate the string */   
  for (count = 0 ; count < HASH_STRING_LENGTH ; count++) {
      buf[count] = buf[pos];
      pos++;
  }
  
  buf[count] = 0;
  return TRUE;
}


bool findHashkeeperHashinLine(char *buf) {
  return (findHashAfterQuotes(buf,3));
}

bool findNSRLHashinLine(char *buf) {
  return (findHashAfterQuotes(buf,9));
} 



/* iLook files have the hash as the first 32 characters. 
   As a result, we can just treat these files like plain hash files  */
bool findiLookHashinLine(char *buf) {
  return (findPlainHashinLine(buf));
}



/* Given an input string buf and the type of file it came from, finds
   the MD5 hash specified in the line if there is one and returns TRUE.
   If there is no valid hash in the line, returns FALSE. */
bool findHashValueinLine(char *buf, int fileType) {
  
  int status;

  switch(fileType) {

  case TYPE_PLAIN:
    status = findPlainHashinLine(buf);
    break;
    
  case TYPE_HASHKEEPER:
    status = findHashkeeperHashinLine(buf);
    break;
    
  case TYPE_NSRL:
    status = findNSRLHashinLine(buf);
    break;
    
  case TYPE_ILOOK:
    status = findiLookHashinLine(buf);
    break;

    /* If we don't know what kind of file this is, we certainly can't
       find a hash in it! (We should never get here, really.) */
  default:
    return FALSE;
  }

  if (!isValidHash(buf))
    return FALSE;

  return status;
}




