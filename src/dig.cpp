// MD5DEEP - dig.c
//
// By Jesse Kornblum and Simson Garfinkel
//
// This is a work of the US Government. In accordance with 17 USC 105,
//  copyright protection is not available for any work of the US Government.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//


/* The functions in this file will recurse through a directory
 * and call hash_file(fn) for every file that needs to be hashes.
 */

// $Id$

#include "main.h"
#include "winpe.h"
#include <iostream>

/*
 * file stat system
 */

// Use a stat function to look up while kind of file this is
// and determine its size if possible
#ifdef _WIN32
#define TSTAT(path,buf) _wstat64(path,buf)
#define TLSTAT(path,buf) _wstat64(path,buf) // no lstat on windows
#else
#define TSTAT(path,buf) stat(path,buf)
#define TLSTAT(path,buf) lstat(path,buf)
#endif

// Returns TRUE if the directory is '.' or '..', otherwise FALSE
static bool is_special_dir(const tstring &d)
{
    return global::make_utf8(d)=="." || global::make_utf8(d)=="..";
}

/* Determine the file type of a structure
 * called by file_type() and should_hash_symlink() 
 */

file_types file_metadata_t::decode_file_type(const struct __stat64 &sb)
{
    if (S_ISREG(sb.st_mode))  return stat_regular;
    if (S_ISDIR(sb.st_mode))  return stat_directory;
    if (S_ISBLK(sb.st_mode))  return stat_block;
    if (S_ISCHR(sb.st_mode))  return stat_character;
    if (S_ISFIFO(sb.st_mode)) return stat_pipe;

#ifdef S_ISSOCK
    if (S_ISSOCK(sb.st_mode)) return stat_socket; // not present on WIN32
#endif

#ifdef S_ISLNK
    if (S_ISLNK(sb.st_mode)) return stat_symlink; // not present on WIN32
#endif   

#ifdef S_ISDOOR
    // Solaris doors are an inter-process communications facility present in Solaris 2.6
    // http://en.wikipedia.org/wiki/Doors_(computing)
    if (S_ISDOOR(sb.st_mode)) return stat_door; 
#endif
    return stat_unknown;
}

/**
 * stat a file and return the appropriate object
 * This would better be done on an open file handle
 */
int file_metadata_t::stat(const tstring &fn,
			  file_metadata_t *m,
			  class display &ocb)
{
  struct __stat64 sb;
  if (::TSTAT(fn.c_str(),&sb)) 
  {
    ocb.error_filename(fn,"%s",strerror(errno));
    return -1;
  }
  m->fileid.dev = sb.st_dev;
#ifdef _WIN32
  BY_HANDLE_FILE_INFORMATION fileinfo;
  HANDLE filehandle = CreateFile(fn.c_str(),
				 0,   // desired access
				 FILE_SHARE_READ,
				 NULL,  
				 OPEN_EXISTING,
				 (FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS),
				 NULL);
  // RBF - Error check CreateFile
  (void)GetFileInformationByHandle(filehandle, &fileinfo);
  CloseHandle(filehandle);
  m->fileid.ino = (((uint64_t)fileinfo.nFileIndexHigh)<<32) | (fileinfo.nFileIndexLow);
#else
  m->fileid.ino = sb.st_ino;
#endif
  m->nlink      = sb.st_nlink;
  m->size       = sb.st_size;
  m->ctime      = sb.st_ctime;
  m->mtime      = sb.st_mtime;
  m->atime      = sb.st_atime;

  return 0;
}


/* Return the 'decoded' file type of a file.
 * If an error is found and ocb is provided, send the error to ocb.
 * If filesize and ctime are provided, give them.
 * Also return the file size and modification time (may be used elsewhere).
 */

file_types state::file_type(const tstring &fn,display *ocb,uint64_t *filesize,
				   timestamp_t *ctime,timestamp_t *mtime,timestamp_t *atime)
{
    struct __stat64 sb;
    memset(&sb,0,sizeof(sb));
    if (TLSTAT(fn.c_str(),&sb))  {
	if(ocb) ocb->error_filename(fn,"%s", strerror(errno));
	return stat_unknown;
    }
    if(ctime) *ctime = sb.st_ctime;
    if(mtime) *mtime = sb.st_mtime;
    if(atime) *atime = sb.st_atime;
    if(filesize){
	if(sb.st_size!=0){
	    *filesize = sb.st_size;
	} else {
	    /*
	     * The stat() function does not return file size for raw devices.
	     * If we have no file size, try to use the find_file_size function,
	     * which calls ioctl.
	     */
	    FILE *f = _tfopen(fn.c_str(),_TEXT("rb"));
	    if(f){
		*filesize = find_file_size(f,ocb);
		fclose(f); f = 0;
	    }
	}
    }
    return file_metadata_t::decode_file_type(sb);
}



/****************************************************************
 *** database of directories we've seen.
 *** originally in cycles.c.
 *** so much smaller now, we put it here.
 ****************************************************************/


void state::done_processing_dir(const tstring &fn_)
{
    tstring fn = global::get_realpath(fn_);
    dir_table_t::iterator pos = dir_table.find(fn);
    if(pos==dir_table.end()){
	ocb.internal_error("%s: Directory '%s' not found in done_processing_dir", progname.c_str(), fn.c_str());
	// will not be reached.
    }
    dir_table.erase(pos);
}


void state::processing_dir(const tstring &fn_)
{
    tstring fn = global::get_realpath(fn_);
    if (dir_table.find(fn)!=dir_table.end())
    {
      ocb.internal_error("%s: Attempt to add existing %s in processing_dir", progname.c_str(), fn.c_str());
      // will not be reached.
    }
    dir_table.insert(fn);
}


bool state::have_processed_dir(const tstring &fn_)
{
    tstring fn = global::get_realpath(fn_);
    return dir_table.find(fn)!=dir_table.end();
}

/****************************************************************
 *** end of cycle processing.
 ****************************************************************/

#ifdef _WIN32
/****************************************************************
 *** WIN32 specific code follows.
 ****************************************************************/


/*
 * On Win32 systems directories are handled... differently
 * Attempting to process d: causes an error, but d:\ does not.
 * Conversely, if you have a directory "foo",
 * attempting to process d:\foo\ causes an error, but d:\foo does not.
 * The following turns d: into d:\ and d:\foo\ into d:\foo 
 * It also leaves d:\ as d:\
 */

static void clean_name_win32(std::wstring &fn)
{
    if (fn.size()<2) return;
    if (fn.size()==2 && fn[1]==_TEXT(':')){
	fn.push_back(_TEXT(DIR_SEPARATOR));
	return;
    }
    if (fn.size()==3 && fn[1]==_TEXT(':')){
	return;
    }
    if (fn[fn.size()-1] == _TEXT(DIR_SEPARATOR)){
	fn.erase(fn.size()-1);
    }
}

static int is_win32_device_file(const std::wstring &fn)
{
    // Specifications for device files came from
    // http://msdn.microsoft.com/en-us/library/aa363858(VS.85).aspx
    // Physical devices (like hard drives) are 
    //   \\.\PhysicalDriveX where X is a digit from 0 to 9
    // Tape devices are \\.\tapeX where X is a digit from 0 to 9
    // Logical volumes are \\.\X: where X is a letter */

    if (!_wcsnicmp(fn.c_str(), _TEXT("\\\\.\\physicaldrive"),17)
	&& (fn.size() == 18) && iswdigit(fn[17]))
	return TRUE;

    if (!_wcsnicmp(fn.c_str(), _TEXT("\\\\.\\tape"),8) &&
	(fn.size() == 9) &&  iswdigit(fn[8]))
	return TRUE;
 
    if ((!_wcsnicmp(fn.c_str(),_TEXT("\\\\.\\"),4)) &&
	(fn.size() == 6) && (iswalpha(fn[4])) && (fn[5] == ':'))
	return TRUE;

    return FALSE;
}

//  Debugging function
void print_last_error(char *function_name)
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
    std::cerr << function_name << "failed with error " << dwLastError
	      << ":" << global::make_utf8(pszMessage) << "\n";
    LocalFree(pszMessage);
}


// An NTFS Junction Point is like a hard link on *nix but only works
// on the same filesystem and only for directories. Unfortunately they
// can also create infinite loops for programs that recurse filesystems.
// See http://blogs.msdn.com/oldnewthing/archive/2004/12/27/332704.aspx 
// for an example of such an infinite loop.
//
// This function detects junction points and returns TRUE if the
// given filename is a junction point. Otherwise it returns FALSE.
bool state::is_junction_point(const std::wstring &fn)
{
  int status = false;
  
  WIN32_FIND_DATA FindFileData;
  HANDLE hFind;
  
  hFind = FindFirstFile(fn.c_str(), &FindFileData);
  if (INVALID_HANDLE_VALUE != hFind)  
  {
    if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)    
    {
      // We're probably going to skip this reparse point, 
      // but not always. (See the logic below.)
      status = true;

      // Tag values come from 
      // http://msdn.microsoft.com/en-us/library/dd541667(prot.20).aspx

      switch (FindFileData.dwReserved0)
      {
      case IO_REPARSE_TAG_MOUNT_POINT:
	ocb.error_filename(fn,"Junction point, skipping");
	break;

      case IO_REPARSE_TAG_SYMLINK:
	// TODO: Maybe have the option to follow symbolic links?
	ocb.error_filename(fn,"Symbolic link, skipping");
	break;

	// TODO: Use label for deduplication reparse point 
	//         when the compiler supports it
	//      case IO_REPARSE_TAG_DEDUP:
      case 0x80000013:
	// This is the reparse point for Data Deduplication
	// See http://blogs.technet.com/b/filecab/archive/2012/05/21/introduction-to-data-deduplication-in-windows-server-2012.aspx
	// Unfortunately the compiler doesn't have this value defined yet.
	status = false;
	break;

      case IO_REPARSE_TAG_SIS:
	// Single Instance Storage
	// "is a system's ability to keep one copy of content that multiple users or computers share"
	// http://blogs.technet.com/b/filecab/archive/2006/02/03/single-instance-store-sis-in-windows-storage-server-r2.aspx
	status = false;
	break;

      default:
	ocb.error_filename(fn,
			   "Unknown reparse point 0x%"PRIx32", skipping",
			   FindFileData.dwReserved0);
	ocb.error_filename(fn,"Please report this to the developers!");

      }
    }
    
    // We don't error check this call as there's nothing to do differently
    // if it fails.
    FindClose(hFind);
  }

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

#endif

/****************************************************************
 *** End of WIN32 specific code.
 ****************************************************************/



/* POSIX version of clean_name */
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
	fn.erase(loc,1);		// erase one of the two slashes
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
	fn.erase(fn.begin()+loc,fn.begin()+loc+2);			// erase
    }
}


/// Removes all "../" references from the absolute path fn 
/// If string contains f/d/e/../a replace it with f/d/a/
static void remove_double_dirs(tstring &fn) {
  tstring search;
  search.push_back(DIR_SEPARATOR);
  search.push_back('.');
  search.push_back('.');
  search.push_back(DIR_SEPARATOR);
  
  while (true) {
    size_t loc = fn.find(search);
    if (loc == tstring::npos) 
      break;

    // See if there is another dir separator before the /../ we just
    // found.
    size_t before = fn.rfind(DIR_SEPARATOR, loc-1);
    if (before == tstring::npos) 
      break;

    // Now delete all between before+1 and loc+3
    fn.erase(fn.begin()+before+1, fn.begin()+loc+4);
  }
}


#ifndef _WIN32
void state::clean_name_posix(std::string &fn)
{
    // We don't need to call these functions when running in Windows
    // as we've already called real_path() on them in main.c. These
    // functions are necessary in *nix so that we can clean up the 
    // path names without removing the names of symbolic links. They
    // are also called when the user has specified an absolute path
    // but has included extra double dots or such.
    //
    // TODO: See if Windows Vista's symbolic links create problems 

    if (!ocb.opt_relative)  {
	remove_double_slash(fn);
	remove_single_dirs(fn);
	remove_double_dirs(fn);
    }
}
#endif

void state::process_dir(const tstring &fn)
{
    _TDIR *current_dir;
    struct _tdirent *entry;

    if(opt_debug) std::cerr << "*** process_dir(" << global::make_utf8(fn) << ")\n";

    if (have_processed_dir(fn)) {
	ocb.error_filename(fn,"symlink creates cycle");
	return ;
    }
  
    if ((current_dir = _topendir(fn.c_str())) == NULL)   {
	ocb.error_filename(fn,"%s", strerror(errno));
	return ;
    }    

    /* 2011-SEP-15 New logic:
     * 1. Open directory.
     * 2. Get a list of all the dir entries.
     * 3. Close the directory.
     * 4. Process them.
     */
    std::vector<tstring> dir_entries;
    while ((entry = _treaddir(current_dir)) != NULL) {
      // ignore . and ..
      if (is_special_dir(entry->d_name)) 
	continue; 
      
      // compute full path
      // don't append if the DIR_SEPARATOR if there is already one there
      tstring new_file = fn;

      if (0 == new_file.size() || new_file[new_file.size()-1]!=DIR_SEPARATOR)
      {
	new_file.push_back(DIR_SEPARATOR);
      }
      new_file.append(entry->d_name);

#ifdef _WIN32
      /// Windows Junction points
      if (is_junction_point(new_file)){
	continue;
      }
#endif

      dir_entries.push_back(new_file);
      
    }
    _tclosedir(current_dir);		// done with this directory

    processing_dir(fn);			// note that we are now processing a directory
    for(std::vector<tstring>::const_iterator it = dir_entries.begin();it!=dir_entries.end();it++){
	dig_normal(*it);
    }
    done_processing_dir(fn);		// note that we are done with this directory
    return ;
}


/* Deterine if a symlink should be hashed or not.
 * Returns TRUE if a symlink should be hashed.
 */
bool state::should_hash_symlink(const tstring &fn, file_types *link_type)
{
    /**
     * We must look at what this symlink points to before we process it.
     * The file_type() function uses lstat to examine the file.
     * Here we use the normal stat to examine what this symlink points to.
     */

    struct __stat64 sb;

    if (TSTAT(fn.c_str(),&sb))  {
	ocb.error_filename(fn,"%s",strerror(errno));
	return false;
    }

    file_types type = file_metadata_t::decode_file_type(sb);

    if (type == stat_directory)  {
	if (mode_recursive){
	    process_dir(fn);
	} else {
	    ocb.error_filename(fn,"Is a directory");
	}
	return false;
    }    

    if (link_type) *link_type = type;
    return true;    
}


/// Returns true if the filename fn is a Windows PE executable
///
/// If the filename is a PE executable but does not have an executable
/// extension, displays an error message. If the file cannot be read,
/// returns false.
///
/// @param fn Filename to examine
bool state::should_hash_winpe(const tstring &fn)
{
  bool executable_extension = has_executable_extension(fn);
  
  FILE * handle = _tfopen(fn.c_str(),_TEXT("rb"));
  if (NULL == handle) {
    ocb.error_filename(fn,"%s", strerror(errno));
    return false;
  }

  unsigned char buffer[PETEST_BUFFER_SIZE] = {0};
  size_t size = fread(buffer,1,PETEST_BUFFER_SIZE,handle);
  fclose(handle);

  bool status = is_pe_file(buffer, size);

  if (status and not executable_extension)
    ocb.error_filename(fn,"Is Windows executable but does not have executable extension");
 
  return status;
}

/*
 * Type should be the result of calling lstat on the file.
 * We want to know what this file is, not what it points to
 * This expert calls itself recursively. The dir_table
 * makes sure that we do not loop.
 */
bool state::should_hash_expert(const tstring &fn, file_types type)
{
    file_types link_type=stat_unknown;
    if (stat_directory == type)  {
	if (mode_recursive){
	    process_dir(fn);
	}
	else {
	    ocb.error_filename(fn,"Is a directory");
	}
	return false;
    }

    if (mode_winpe)  {
	// The user could have requested PE files *and* something else
	// therefore we don't return false here if the file is not a PE.
	// Note that we have to check for directories first!
	if (should_hash_winpe(fn)){
	    return true;
	}
    }

    switch(type) {
	// We can't just return s->mode & mode_X because mode_X is
	// a 64-bit value. When that value gets converted back to int,
	// the high part of it is lost. 
	
#define RETURN_IF_MODE(A) if (A) return true; break;
    case stat_directory:
	// This case should be handled above. This statement is 
	// here to avoid compiler warnings
	ocb.internal_error("Did not handle directory entry in should_hash_expert()");
	
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
	
	if (!(mode_symlink)) return false;
	if (should_hash_symlink(fn,&link_type))    {
	    return should_hash_expert(fn,link_type);
	}
	return false;
    case stat_unknown:
	ocb.error_filename(fn,"unknown file type");
	return false;
    }
    return false;
}


/*
 * should_hash confusingly returns true if a file should be hashed,
 * but if it is called with a directory it recursively hashes it.
 */

bool state::should_hash(const tstring &fn)
{
    file_types type = state::file_type(fn,&ocb,0,0,0,0);
  
    if (mode_expert) 
      return should_hash_expert(fn,type);

    if (type == stat_directory)  {
	if (mode_recursive){
	    process_dir(fn);
	} else {
	    ocb.error_filename(fn,"Is a directory");
	}
	return false;
    }

    if (type == stat_symlink){
	return should_hash_symlink(fn,NULL);
    }

    if (type == stat_unknown){
	return false;
    }

    // By default we hash anything we can't identify as a "bad thing"
    return true;
}

// Search through a directory and its subdirectory for files to hash 
void state::dig_normal(const tstring &fn_) {
  // local copy will be modified
  tstring fn(fn_);			
  if (opt_debug) 
    ocb.status("*** state::dig_normal(%s)",global::make_utf8(fn).c_str());
#ifdef _WIN32
  clean_name_win32(fn);
#else
  clean_name_posix(fn);
#endif
  if (opt_debug) 
    ocb.status("*** cleaned:%s",global::make_utf8(fn).c_str());
  if (should_hash(fn))
    ocb.hash_file(fn);
}


#ifdef _WIN32
/**
 * Extract the directory name from a string and return it.
 * Do not include the final 
 */

std::wstring  win32_dirname(const std::wstring &fn)
{
    size_t loc = fn.rfind(DIR_SEPARATOR);
    if (loc==tstring::npos)
    {
      return tstring(); // return empty string
    }
    return fn.substr(0,loc);
}


/**
 * dig_win32 ensures the program processes files with Unicode filenames.
 * They are not processed using the standard opendir/readdir commands.
 * It also handles the case of asterisks on the command line being used to refer to files with
 * Unicode filenames.
 */

void state::dig_win32(const std::wstring &fn)
{
    WIN32_FIND_DATA FindFileData;
    HANDLE hFind;

    if(opt_debug) ocb.status("*** state::dig_win32(%s)",
			     global::make_utf8(fn).c_str());

    if (is_win32_device_file(fn)){
	ocb.hash_file(fn);
	return ;
    }

    // Filenames without wildcards can be processed by the
    // normal recursion code.
    size_t asterisk = fn.find(L'*');
    size_t question  = fn.find(fn,L'?');
    if (asterisk==std::wstring::npos && question==std::wstring::npos){
	dig_normal(fn);
	return;
    }
  
    hFind = FindFirstFile(fn.c_str(), &FindFileData);
    if (INVALID_HANDLE_VALUE == hFind)  {
	ocb.error_filename(fn,"No such file or directory");
	return;
    }
  
#define FATAL_ERROR_UNK(A) if (NULL == A) fatal_error("%s: %s", progname.c_str(), strerror(errno));
#define FATAL_ERROR_MEM(A) if (NULL == A) fatal_error("%s: Out of memory",progname.c_str());
  
    tstring dirname = win32_dirname(fn);
    if(dirname.size()>0) dirname += _TEXT(DIR_SEPARATOR);
  
    int rc = 1;
    while (rc!=0)  {
	if (!(is_special_dir(FindFileData.cFileName)))    {
	    // The filename we've found doesn't include any path information.
	    // We have to add it back in manually. Thankfully Windows doesn't
	    // allow wildcards in the early part of the path. For example,
	    // we will never see:  c:\bin\*\tools 
	    //
	    // Because the wildcard is always in the last part of the input
	    // (e.g. c:\bin\*.exe) we can use the original dirname, combined
	    // with the filename we've found, to make the new filename. 
      
	    std::wstring new_fn;

	    new_fn = dirname + FindFileData.cFileName;
	    if (!ocb.opt_relative) {
		new_fn = global::get_realpath(new_fn);
	    }
      
	    if (!(is_junction_point(new_fn))) dig_normal(new_fn); 
	}
    
	rc = FindNextFile(hFind, &FindFileData);
    }
  
    // rc now equals zero, we can't find the next file

    if (ERROR_NO_MORE_FILES != GetLastError())  {
	// The Windows API for getting an intelligible error message
	// is beserk. Rather than play their silly games, we 
	// acknowledge that an unknown error occured and hope we
	// can continue.
	ocb.error_filename(fn,"Unknown error while expanding wildcard");
	return;
    }
  
    rc = FindClose(hFind);
    if (0 == rc)  {
	ocb.error_filename(fn,"Unknown error while cleaning up wildcard expansion");
    }
    if(opt_debug) ocb.status("state::dig_win32(%s)",global::make_utf8(fn).c_str());
}
#endif

/**
 * Test the string manipulation routines.
 */

inline std::ostream & operator << (std::ostream &os,const struct __stat64 sb) {
    os << "size=" << sb.st_size << " ctime=" << sb.st_ctime ;
    return os;
}

void state::dig_self_test()
{
    struct __stat64 sb;
    std::cerr << "dig_self_test\n";

    memset(&sb,0,sizeof(sb));
    std::cerr << "check stat 1: "
	      << TLSTAT(_T("z:\\simsong on my mac\\md5deep\\branches\\version4\\hashdeep\\md5.cpp"),&sb)
	      << " " << sb << "\n";

    memset(&sb,0,sizeof(sb));
    std::cerr <<  "check stat 2: "
	      <<  TLSTAT(_T("c:\\autoexec.bat"),&sb)
	      << " " << sb << "\n";

    memset(&sb,0,sizeof(sb));
    std::cerr <<  "check stat 3: "
	      <<  TLSTAT(_T("/etc/resolv.conf"),&sb)
	      << " " << sb << "\n";

    tstring fn(_T("this is"));
    fn.push_back(DIR_SEPARATOR);
    fn.push_back(DIR_SEPARATOR);
    fn += _T("a test");

    tstring fn2(fn);
    remove_double_slash(fn2);

    std::cerr << "remove_double_slash(" << fn << ")="<<fn2<<"\n";

    fn = _T("this is");
    fn.push_back(DIR_SEPARATOR);
    fn.push_back('.');
    fn.push_back(DIR_SEPARATOR);
    fn += _T("a test");

    remove_single_dirs(fn2);
    std::cerr << "remove_single_dirs(" << fn << ")="<<fn2<<"\n";

    fn = _T("this is");
    fn.push_back(DIR_SEPARATOR);
    fn += _T("a mistake");
    fn.push_back(DIR_SEPARATOR);
    fn.push_back('.');
    fn.push_back('.');
    fn.push_back(DIR_SEPARATOR);
    fn += _T("a test");
    fn2 = fn;

    remove_double_dirs(fn2);
    std::cerr << "remove_double_dirs(" << fn << ")="<<fn2<<"\n";
    std::cerr << "is_special_dir(.)=" << is_special_dir(_T(".")) << "\n";
    std::cerr << "is_special_dir(..)=" << is_special_dir(_T("..")) << "\n";

    tstring names[] = {_T("dig.cpp"),
		       _T("."),_T("/dev/null"),_T("/dev/tty"),
		       _T("../testfiles/symlinktest/dir1/dir1"),_T("")};

    state s;
    for(int i=0;names[i].size()>0;i++){
	uint64_t stat_bytes;
	timestamp_t ctime;
	timestamp_t mtime;
	timestamp_t atime;
	file_types ft = s.file_type(names[i],&s.ocb,&stat_bytes,&ctime,&mtime,&atime);
	std::cerr << "file_type(" << names[i] << ")="
		  << (int)ft << " size=" << stat_bytes << " ctime=" << ctime << "\n";
    }
}
