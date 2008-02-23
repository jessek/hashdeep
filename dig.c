
/* MD5DEEP - dig.c
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

void remove_double_slash(char *fn)
{
  int pos;
  for (pos = 0 ; pos < strlen(fn) ; pos++)
    {
      if (fn[pos] == DIR_SEPARATOR && fn[pos+1] == DIR_SEPARATOR)
	{
	  shift_string(fn,pos,pos+1);
	  --pos;
	}
    }
}


void remove_single_dirs(char *fn)
{
  int pos, chars_found = 0;
  for (pos = 0 ; pos < strlen(fn) ; pos++)
    {

      /* Catch strings that end with "/." */
      if (pos > 0 && fn[pos+1] == 0 &&
	  fn[pos-1] == DIR_SEPARATOR && fn[pos] == '.')
	fn[pos] = 0;

      if (fn[pos] == '.' && fn[pos+1] == DIR_SEPARATOR)
	{
	  if (chars_found && fn[pos-1] == DIR_SEPARATOR)
	    {
	      shift_string(fn,pos,pos+2);

	      /* In case we have ././ we shift back one! */
	      --pos;
	    }
	}
    else 
      ++chars_found;
    }
}

/* Removes all "../" references from the absolute path fn */
void remove_double_dirs(char *fn)
{
  int pos, next_dir;

  for (pos = 0; pos < strlen(fn) ; pos++)
  {
    if (fn[pos] == '.' && fn[pos+1] == '.')
    {
      if (pos > 0)
      {

	/* We have to keep this next if and the one above separate.
	   If not, we can't tell later on if the pos <= 0 or
	   that the previous character was a DIR_SEPARATOR.
	   This matters when we're looking at ..foo/ as an input */
	
	if (fn[pos-1] == DIR_SEPARATOR)
	{
	  
	  next_dir = pos + 2;
	  
	  /* Back up to just before the previous DIR_SEPARATOR
	     unless we're already at the start of the string */
	  if (pos > 1)
	    pos -= 2;
	  else
	    pos = 0;
	  
	  while (fn[pos] != DIR_SEPARATOR && pos > 0)
	    --pos;
	  
	  switch(fn[next_dir])
	  {
	  case DIR_SEPARATOR:
	    shift_string(fn,pos,next_dir);
	    break;
	    
	  case 0:
	    /* If we have /.. ending the filename */
	    fn[pos+1] = 0;
	    break;
	    
	    /* If we have ..foo, we should do nothing, but skip 
	       over these double dots */
	  default:
	    pos = next_dir;
	  }
	}
      }
      
      /* If we have two dots starting off the string, we should prepend
	 a DIR_SEPARATOR and ignore the two dots. That is:
	 from the root directory the path ../foo is really just /foo */
    
      else 
      {
	fn[pos] = DIR_SEPARATOR;
	shift_string(fn,pos+1,pos+3);
      }
    }
  }
}


/* On Win32 systems directories are handled... differently
   Attempting to process d: causes an error, but d:\ does not.
   Conversely, if you have a directory "foo",
   attempting to process d:\foo\ causes an error, but d:\foo does not.
   The following turns d: into d:\ and d:\foo\ into d:\foo */

#ifdef __WIN32
void clean_name_win32(char *fn)
{
  unsigned int length = strlen(fn);

  if (length < 2)
    return;

  if (length == 2 && fn[1] == ':')
    {
      fn[length+1] = 0;
      fn[length]   = DIR_SEPARATOR;
      return;
    }

  if (fn[length-1] == DIR_SEPARATOR && length != 3)
    {
      fn[length - 1] = 0;
    }
}

int check_special_files(char *fn)
{

  /* Specifications for special files came from
     http://msdn.microsoft.com/library/default.asp?url=/library/en-us/fileio/base/createfile.asp
   */

  /* Check for physical devices */
  if (!strncasecmp(fn,"\\\\.\\physicaldrive",17))
  {
    /* Format for physical devices is \\.\PhysicalDriveX where X 
       is a digit from 0 to 9. */
    if ((strlen(fn) == 18) && isdigit(fn[17]))
      return TRUE;
  }

  /* Check for tape devices */
  if (!strncasecmp(fn,"\\\\.\\tape",8))
  {
    /* Format for tape devices is \\.\tapeX where X is a digit from 0 to 9 */
    if ((strlen(fn) == 9) && isdigit(fn[8]))
      return TRUE;
  }
  
  return FALSE;
}

#endif  /* ifdef __WIN32 */


int clean_name(char *fn)
{
#ifdef __WIN32
  if (check_special_files(fn))
    return TRUE;
#endif

  remove_double_slash(fn);
  remove_single_dirs(fn);
  remove_double_dirs(fn);

#ifdef __WIN32
  clean_name_win32(fn);
#endif

  return FALSE;
}


int is_special_dir(char *d)
{
  return ((!strncmp(d,".",1) && (strlen(d) == 1)) ||
          (!strncmp(d,"..",2) && (strlen(d) == 2)));
}

void process(off_t mode, char *input);

void process_dir(off_t mode, char *fn)
{
  char *new_file = (char *)malloc(sizeof(char) * PATH_MAX);
  DIR *current_dir;
  struct dirent *entry;

  if (have_processed_dir(mode,fn))
    print_error(mode,fn,"symlink creates cycle");
  else
  {
    
    if (!processing_dir(mode,fn))
    {
      print_error(mode,fn,"internal error. Contact developer!");
      exit(-1);
    }
    
    if ((current_dir = opendir(fn)) == NULL) 
    {
      print_error(mode,fn,strerror(errno));
      return;
    }    
    
    while ((entry = readdir(current_dir)) != NULL) 
    {
      if (is_special_dir(entry->d_name))
	continue;
      
      snprintf(new_file,PATH_MAX,"%s%c%s", fn,DIR_SEPARATOR,entry->d_name);
      process(mode,new_file);
    }

    closedir(current_dir);
    
    if (!done_processing_dir(mode,fn))
    {
      print_error(mode,fn,"internal error. Contact developer!");
      exit(-1);
    }

  }
  
  free(new_file);
}



int file_type_helper(struct stat sb)
{
  if (S_ISREG(sb.st_mode))
    return file_regular;
  
  if (S_ISDIR(sb.st_mode))
    return file_directory;
  
  if (S_ISBLK(sb.st_mode))
    return file_block;
  
  if (S_ISCHR(sb.st_mode))
    return file_character;
  
  /* These file types do not exist in Win32 */
#ifndef __WIN32
  
  if (S_ISFIFO(sb.st_mode))
    return file_pipe;

  if (S_ISSOCK(sb.st_mode))
    return file_socket;
  
  if (S_ISLNK(sb.st_mode))
    return file_symlink;  
#endif   /* ifndef __WIN32 */


  /* Used to detect Solaris doors */
#ifdef S_IFDOOR
#ifdef S_ISDOOR
  if (S_ISDOOR(sb.st_mode))
    return file_door;
#endif
#endif

  return file_unknown;
}

int file_type(off_t mode, char *fn)
{
  struct stat sb;

  if (lstat(fn,&sb))  
  {
    print_error(mode,fn,strerror(errno));
    return file_unknown;
  }
  return file_type_helper(sb);
}


int should_hash(off_t mode, char *fn);  

int should_hash_expert(off_t mode, char *fn, int type)
{
  switch(type)
  {
  case file_regular:
    return (M_REGULAR(mode));
    
  case file_block:
    return (M_BLOCK(mode));
    
  case file_character:
    return (M_CHARACTER(mode));
    
  case file_pipe:
    return (M_PIPE(mode));
    
  case file_socket:
    return (M_SOCKET(mode));
    
  case file_door:
    return (M_DOOR(mode));
    
  case file_symlink:
    
    /* We're not going to hash the symlink itself, but rather what 
       it points to. To make that happen we need to call the 
       regular should_hash function to distinguish between regular 
       files and directories. But we have to strip the expert mode
       out of the mode or we'll just make an infinite loop! */

    if (M_SYMLINK(mode))
      return (should_hash((mode) & ~(mode_expert),fn));
  }
  
  return FALSE;
}


int should_hash_symlink(off_t mode, char *fn)
{
  int type;
  struct stat sb;

   /* We must look at what this symlink points to before we process it.
      Symlinks to directories can cause cycles */
  if (stat(fn,&sb))
  {
    print_error(mode,fn,strerror(errno));
    return FALSE;
  }
  
  type = file_type_helper(sb);

  if (type != file_directory)
    return TRUE;
    
  if (M_RECURSIVE(mode))
  {
    process_dir(mode,fn);
    return FALSE;
  }
  else
  {
    print_error(mode,fn,"Is a directory");
    return FALSE;
  }
}
  

  

int should_hash(off_t mode, char *fn)
{
  int type = file_type(mode,fn);

  if (type == file_directory)
  {
    if (M_RECURSIVE(mode)) 
      process_dir(mode,fn);
    else 
      print_error(mode,fn,"Is a directory");
    
    return FALSE;
  }
  
  if (M_EXPERT(mode))
    return should_hash_expert(mode,fn,type);

#ifndef __WIN32
  if (type == file_symlink)
    return should_hash_symlink(mode,fn);
#endif

  if (type == file_unknown)
    return FALSE;

  /* By default we hash anything we can't identify as a "bad thing" */
  return TRUE;
}


void process(off_t mode, char *fn)
{
  /* We still have to clean filenames on Windows just in case we
     are trying to process c:\\, which can happen even after
     passing the filename through realpath (_fullpath, whatever). 
     We also check for special filenames like \\.\PhysicalDrive0. 
     If we find one of those, we should process this file no matter what. */
  if (clean_name(fn))
    hash(mode,fn);
  else 
    if (should_hash(mode,fn))
      hash(mode,fn);
}
