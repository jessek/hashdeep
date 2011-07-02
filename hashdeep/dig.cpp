
// MD5DEEP - dig.c
//
// By Jesse Kornblum
//
// This is a work of the US Government. In accordance with 17 USC 105,
//  copyright protection is not available for any work of the US Government.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
//

// $Id$

#include "main.h"

/*
 * These functions have code to handle WIN32, but they are never called on WIN32 systems.
 */
static void remove_double_slash(tstring &fn)
{
    tstring search;
    search.push_back(DIR_SEPARATOR);
    search.push_back(DIR_SEPARATOR);
#ifdef _WIN32
    // On Windows, we have to allow the first two characters to be slashes
    // to account for UNC paths. e.g. \\SERVER\dir\path
    // So on windows we ignore the first character
    size_t start = 1;
#else    
    size_t start = 0;
#endif

    while(true){
	size_t loc = fn.find(search,start);
	if(loc==tstring::npos) break;	// no more to find
	fn.erase(loc,2);			// erase
    }
}


/*
 * remove any /./
 */
static void remove_single_dirs(tstring &fn)
{
    tstring search;
    search.push_back(DIR_SEPARATOR);
    search.push_back('.');
    search.push_back(DIR_SEPARATOR);

    while(true){
	size_t loc = fn.find(search);
	if(loc==tstring::npos) break;	// no more to find
	fn.erase(loc,3);			// erase
    }
}


// Removes all "../" references from the absolute path fn 
// If string contains f/d/e/../a replace it with f/d/a/

void remove_double_dirs(tstring &fn)
{
    tstring search;
    search.push_back(DIR_SEPARATOR);
    search.push_back('.');
    search.push_back('.');
    search.push_back(DIR_SEPARATOR);

    while(true){
	size_t loc = fn.rfind(search);
	if(loc==tstring::npos) break;
	/* See if there is another dir separator */
	size_t before = fn.rfind(DIR_SEPARATOR,loc-1);
	if(before==tstring::npos) break;

	/* Now delete all between before+1 and loc+3 */
	fn.erase(before+1,(loc+3)-(before+1));
    }
}



// On Win32 systems directories are handled... differently
// Attempting to process d: causes an error, but d:\ does not.
// Conversely, if you have a directory "foo",
// attempting to process d:\foo\ causes an error, but d:\foo does not.
// The following turns d: into d:\ and d:\foo\ into d:\foo 

#ifdef _WIN32
static void clean_name_win32(tstring &fn)
{
    size_t length = _tcslen(fn);

    if (length < 2)
	return;

    if (length == 2 && fn[1] == _TEXT(':'))
	{
	    fn[length+1] = 0;
	    fn[length]   = _TEXT(DIR_SEPARATOR);
	    return;
	}

    if (fn[length-1] == _TEXT(DIR_SEPARATOR) && length != 3)
	{
	    fn[length - 1] = 0;
	}
}

static int is_win32_device_file(tstring &fn)
{
    // Specifications for device files came from
    // http://msdn.microsoft.com/en-us/library/aa363858(VS.85).aspx
    // Physical devices (like hard drives) are 
    //   \\.\PhysicalDriveX where X is a digit from 0 to 9
    // Tape devices are \\.\tapeX where X is a digit from 0 to 9
    // Logical volumes are \\.\X: where X is a letter */

    if (!_tcsnicmp(fn, _TEXT("\\\\.\\physicaldrive"),17) &&
	(_tcslen(fn) == 18) && 
	isdigit(fn[17]))
	return TRUE;

    if (!_tcsnicmp(fn, _TEXT("\\\\.\\tape"),8) &&
	(_tcslen(fn) == 9) && 
	isdigit(fn[8]))
	return TRUE;
 
    if ((!_tcsnicmp(fn,_TEXT("\\\\.\\"),4)) &&
	(_tcslen(fn) == 6) &&
	(isalpha(fn[4])) &&
	(fn[5] == ':'))
	return TRUE;

    return FALSE;
}

#endif  // ifdef _WIN32 


static void clean_name(state *s, tstring &fn)
{
#ifdef _WIN32
    clean_name_win32(fn);
#else

    // We don't need to call these functions when running in Windows
    // as we've already called real_path() on them in main.c. These
    // functions are necessary in *nix so that we can clean up the 
    // path names without removing the names of symbolic links. They
    // are also called when the user has specified an absolute path
    // but has included extra double dots or such.
    //
    // TODO: See if Windows Vista's symbolic links create problems 

    if (!(s->mode & mode_relative))  {
	remove_double_slash(fn);
	remove_single_dirs(fn);
	remove_double_dirs(fn);
    }
#endif
}


//  Debugging function
#ifdef _WIN32
void print_last_error(char * function_name)
{
    // Copied from http://msdn.microsoft.com/en-us/library/ms680582(VS.85).aspx
    // Retrieve the system error message for the last-error code

    LPTSTR pszMessage;
    DWORD dwLastError = GetLastError(); 

    FormatMessage((FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		   FORMAT_MESSAGE_FROM_SYSTEM |
		   FORMAT_MESSAGE_IGNORE_INSERTS),
		  NULL,
		  dwLastError,
		  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		  (LPTSTR) &pszMessage,
		  0, NULL );
  
    // Display the error message and exit the process
    fprintf(stdout,"%s failed with error %ld: ", function_name, dwLastError);
    output_filename(stdout,pszMessage);
  
    LocalFree(pszMessage);
}
#endif


// An NTFS Junction Point is like a hard link on *nix but only works
// on the same filesystem and only for directories. Unfortunately they
// can also create infinite loops for programs that recurse filesystems.
// See http://blogs.msdn.com/oldnewthing/archive/2004/12/27/332704.aspx 
// for an example of such an infinite loop.
//
// This function detects junction points and returns TRUE if the
// given filename is a junction point. Otherwise it returns FALSE.
static bool is_junction_point(const tstring &fn)
{
    int status = FALSE;

#ifdef _WIN32
    WIN32_FIND_DATA FindFileData;
    HANDLE hFind;

    hFind = FindFirstFile(fn, &FindFileData);
    if (INVALID_HANDLE_VALUE != hFind)  {
	if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)    {
	    // We're going to skip this reparse point no matter what,
	    // but we may want to display a message just in case.
	    // TODO: Maybe have the option to follow symbolic links?
	    status = TRUE;

	    if (IO_REPARSE_TAG_MOUNT_POINT == FindFileData.dwReserved0)
		{
		    print_error_filename(fn,"Junction point, skipping");
		}
	    else if (IO_REPARSE_TAG_SYMLINK == FindFileData.dwReserved0)
		{
		    print_error_filename(fn,"Symbolic link, skipping");
		}	
	    else 
		{
		    print_error_filename(fn,"Unknown reparse point 0x%"PRIx32", skipping",
					 FindFileData.dwReserved0);
		}
	}

	// We don't error check this call as there's nothing to do differently
	// if it fails.
	FindClose(hFind);
    }
#endif

    return status;
}

// This is experimental code for reparse point process
// We don't use it yet, but I don't want to delete it
// until I know what I'm doing. (jk 1 Mar 2009)
/*
  #include <ddk/ntifs.h>
  
  if (status)
  {
  HANDLE hFile = CreateFile(fn,
  0,   // desired access
  FILE_SHARE_READ,
  NULL,  
  OPEN_EXISTING,
  (FILE_FLAG_OPEN_REPARSE_POINT | 
  FILE_FLAG_BACKUP_SEMANTICS),
  NULL);

  if (INVALID_HANDLE_VALUE == hFile)
  {
  print_last_error(L"CreateFile");
  }
  else
  {
  print_status("Opened ok!");

  REPARSE_DATA_BUFFER buf;
  uint8_t bytesReturned;
  LPOVERLAPPED ov;

  int rc = DeviceIoControl(hFile,
  FSCTL_GET_REPARSE_POINT,
  NULL,
  0,
  &buf,
  MAXIMUM_REPARSE_DATA_BUFFER_SIZE,
  &bytesReturned,
  ov);
  if (!rc)
  {
  print_last_error(L"DeviceIoControl");
  }

  CloseHandle(hFile);
  }
  }

  FindClose(hFind);
*/




// Returns TRUE if the directory is '.' or '..', otherwise FALSE
static bool is_special_dir(const tstring &d)
{
    return main::make_utf8(d)=="." || main::make_utf8(d)=="..";
}



static int process_dir(state *s, const tstring &fn)
{
    int return_value = STATUS_OK;
    _TDIR *current_dir;
    struct _tdirent *entry;

    //  print_status (_TEXT("Called process_dir(%s)"), fn);

    if (have_processed_dir(fn)) {
	print_error_filename(fn,"symlink creates cycle");
	return STATUS_OK;
    }

    processing_dir(fn);			// note that we are now processing a directory
  
    if ((current_dir = _topendir(fn.c_str())) == NULL)   {
	print_error_filename(fn,"%s", strerror(errno));
	return STATUS_OK;
    }    

    while ((entry = _treaddir(current_dir)) != NULL)   {
	if (is_special_dir(entry->d_name))
	    continue;
    
	tstring new_file = fn + DIR_SEPARATOR + entry->d_name;
	if (is_junction_point(new_file)){
	    continue;
	}
	return_value = dig_normal(s,new_file);

    }
    _tclosedir(current_dir);
  
    done_processing_dir(fn);		// note that we are done with this directory

    return return_value;
}


static file_types file_type_helper(_tstat_t sb)
{
    if (S_ISREG(sb.st_mode)) return stat_regular;
  
    if (S_ISDIR(sb.st_mode)) return stat_directory;
  
    if (S_ISBLK(sb.st_mode)) return stat_block;
  
    if (S_ISCHR(sb.st_mode)) return stat_character;
  
    if (S_ISFIFO(sb.st_mode)) return stat_pipe;

    // These file types do not exist in Win32 
#ifndef _WIN32
    if (S_ISSOCK(sb.st_mode)) return stat_socket;
  
    if (S_ISLNK(sb.st_mode)) return stat_symlink;  
#endif   // ifndef _WIN32 

    // Used to detect Solaris doors 
#ifdef S_IFDOOR
#ifdef S_ISDOOR
    if (S_ISDOOR(sb.st_mode)) return stat_door;
#endif
#endif

    return stat_unknown;
}


// Use a stat function to look up while kind of file this is
// and, if possible, it's size.
static file_types file_type(state *s,file_data_hasher_t *fdht, const tstring &fn)
{
    _tstat_t sb;

    if (_lstat(fn.c_str(),&sb))  {
	print_error_filename(fn,"%s", strerror(errno));
	return stat_unknown;
    }

    fdht->stat_bytes = sb.st_size;

    // On Win32 this should be the creation time, but on all other systems
    // it will be the change time.
    fdht->timestamp = sb.st_ctime; 

    return file_type_helper(sb);
}

static int should_hash_symlink(state *s, const tstring &fn, file_types *link_type);


// Type should be the result of calling lstat on the file. We want to
// know what this file is, not what it points to
static int should_hash_expert(state *s, const tstring &fn, file_types type)
{
    file_types link_type=stat_unknown;
    switch(type)  {

    case stat_directory:
	if (s->mode & mode_recursive){
	    process_dir(s,fn);
	}
	else {
	    print_error_filename(fn,"Is a directory");
	}
	return FALSE;

	// We can't just return s->mode & mode_X because mode_X is
	// a 64-bit value. When that value gets converted back to int,
	// the high part of it is lost. 

#define RETURN_IF_MODE(A) if (s->mode & A) return TRUE; break;
    case stat_regular:   RETURN_IF_MODE(mode_regular);
    case stat_block:     RETURN_IF_MODE(mode_block);
    case stat_character: RETURN_IF_MODE(mode_character);
    case stat_pipe:      RETURN_IF_MODE(mode_pipe);
    case stat_socket:    RETURN_IF_MODE(mode_socket);
    case stat_door:      RETURN_IF_MODE(mode_door);
    case stat_symlink: 

	//  Although it might appear that we need nothing more than
	//     return (s->mode & mode_symlink);
	// that doesn't work. That logic gets into trouble when we're
	// running in recursive mode on a symlink to a directory.
	// The program attempts to open the directory entry itself
	// and gets into an infinite loop.

	if (!(s->mode & mode_symlink)) return FALSE;
	if (should_hash_symlink(s,fn,&link_type)){
	    return should_hash_expert(s,fn,link_type);
	}
	return FALSE;
    case stat_unknown:
	print_error_filename(fn,"unknown file type");
	return FALSE;
    }
    return FALSE;
}


static int should_hash_symlink(state *s, const tstring &fn, file_types *link_type)
{
    file_types type;
    _tstat_t sb;

    // We must look at what this symlink points to before we process it.
    // The normal file_type function uses lstat to examine the file,
    // we use stat to examine what this symlink points to. 
    if (_sstat(fn.c_str(),&sb))  {
	print_error_filename(fn,"%s",strerror(errno));
	return FALSE;
    }

    type = file_type_helper(sb);

    if (type == stat_directory)  {
	if (s->mode & mode_recursive)
	    process_dir(s,fn);
	else    {
	    print_error_filename(fn,"Is a directory");
	}
	return FALSE;
    }    

    if (link_type) *link_type = type;
    return TRUE;    
}
    

static int should_hash(state *s, file_data_hasher_t *fdht,const tstring &fn)
{
    file_types type;

    // We must reset the number of bytes in each file processed
    // so that we can tell if fstat reads the number successfully
    fdht->stat_bytes = UNKNOWN_FILE_SIZE;
    fdht->timestamp   = 0;

    type = file_type(s,fdht,fn);
  
    if (s->mode & mode_expert){
	return (should_hash_expert(s,fn,type));
    }

    if (type == stat_directory)  {
	if (s->mode & mode_recursive){
	    process_dir(s,fn);
	}    else     {
	    print_error_filename(fn,"Is a directory");
	}
	return FALSE;
    }

#ifndef _WIN32
    if (type == stat_symlink)
	return should_hash_symlink(s,fn,NULL);
#endif

    if (type == stat_unknown){
	return FALSE;
    }

    // By default we hash anything we can't identify as a "bad thing"
    return TRUE;
}


// RBF - Standardize return values for this function and audit functions
// This function returns FALSE. hash_file, called above, returns STATUS_OK
// process_win32 also returns STATUS_OK. 
// display_audit_results, used by hashdeep, returns EXIT_SUCCESS/FAILURE.
// Pick one and stay with it!
int dig_normal(state *s, const tstring &fn)
{
    int ret = FALSE;
    file_data_hasher_t *fdht = new file_data_hasher_t(s->mode & mode_piecewise);
    fdht->retain();

    tstring fn2(fn);
    clean_name(s,fn2);

    if (should_hash(s,fdht,fn2)){
	ret = s->hash_file(fdht,fn2);
    }
    fdht->release();
    return ret;
}


#ifdef _WIN32
// extract the directory name
// Works like dirname(3), but accepts a TCHAR* instead of char*
int my_dirname(const tstring &c)
{
    TCHAR *tmp;


    // If there are no DIR_SEPARATORs in the directory name, then the 
    // directory name should be the empty string
    tmp = _tcsrchr(c,DIR_SEPARATOR);
    if (NULL != tmp)
	tmp[1] = 0;
    else
	c[0] = 0;

    return FALSE;
}


int dig_win32(state *s, const tstring &fn)
{
    int rc, status = STATUS_OK;
    TCHAR *asterisk, *question;
    WIN32_FIND_DATA FindFileData;
    HANDLE hFind;

    //  print_status("Called process_win32(%S)", fn);

    if (is_win32_device_file(fn)){
	return (hash_file(s,fn));
    }

    // Filenames without wildcards can be processed by the
    // normal recursion code.
    asterisk = _tcschr(fn,L'*');
    question = _tcschr(fn,L'?');
    if (NULL == asterisk && NULL == question)
	return (dig_normal(s,fn));
  
    hFind = FindFirstFile(fn, &FindFileData);
    if (INVALID_HANDLE_VALUE == hFind)  {
	print_error_filename(fn,"No such file or directory");
	return STATUS_OK;
    }
  
#define FATAL_ERROR_UNK(A) if (NULL == A) fatal_error("%s: %s", __progname, strerror(errno));
#define FATAL_ERROR_MEM(A) if (NULL == A) fatal_error("%s: Out of memory", __progname);
  
    TCHAR new_fn[PATH_MAX];
    TCHAR dirname[PATH_MAX];
  
    _tcsncpy(dirname,fn,sizeof(dirname));
  
    my_dirname(dirname);
  
    rc = 1;
    while (0 != rc)  {
	if (!(is_special_dir(FindFileData.cFileName)))    {
	    // The filename we've found doesn't include any path information.
	    // We have to add it back in manually. Thankfully Windows doesn't
	    // allow wildcards in the early part of the path. For example,
	    // we will never see:  c:\bin\*\tools 
	    //
	    // Because the wildcard is always in the last part of the input
	    // (e.g. c:\bin\*.exe) we can use the original dirname, combined
	    // with the filename we've found, to make the new filename. 
      
	    if (s->mode & mode_relative)      {
		_sntprintf(new_fn,sizeof(new_fn),
			   _TEXT("%s%s"), dirname, FindFileData.cFileName);
	    }
	    else      {	  
		TCHAR tmp[PATH_MAX];
		_sntprintf(tmp,sizeof(tmp),_TEXT("%s%s"),dirname,FindFileData.cFileName);
		_wfullpath(new_fn,tmp,PATH_MAX);
	    }
      
	    if (!(is_junction_point(new_fn))) dig_normal(s,new_fn); 
	}
    
	rc = FindNextFile(hFind, &FindFileData);
    }
  
    // rc now equals zero, we can't find the next file

    if (ERROR_NO_MORE_FILES != GetLastError())  {
	// The Windows API for getting an intelligible error message
	// is beserk. Rather than play their silly games, we 
	// acknowledge that an unknown error occured and hope we
	// can continue.
	print_error_filename(fn,"Unknown error while expanding wildcard");
	return STATUS_OK;
    }
  
    rc = FindClose(hFind);
    if (0 == rc)  {
	print_error_filename(
			     fn,
			     "Unknown error while cleaning up wildcard expansion");
    }
    return status;
}
#endif




