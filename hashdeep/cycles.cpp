/**
 * Directory cycle detection.
 *
 * processing_dir() - called by dig.cpp when a directory starts being processed.
 *                    adds the directory to a table of directories, which is currently
 *                    a linked list but should probably be a C++ set of strings.
 * done_processing_dir() - removes a directory from the list.
 *                    Generates an error if directory to be removed is not present (because
 *                    that means we entered it twice.)
 * have_processed_dir() - called in dig.cpp to determine if a directory that's being
 *                       processed should still be processed.
 *
 * 
 * Revision history:
 * Version 3 - Present
 * Version 4 - Rewritten by Simson Garfinkel to hold the full path
 *             in a set of UTF-8 strings.
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
 * $Id$
 */

#include "main.h"
#include <set>

class dir_table_t : std::set<std::string>{
};

dir_table_t dir_table;

void done_processing_dir(const tstring &fn)
{
    std::string rp = main::get_realpath8(fn);
    dir_table_t::iterator pos = dir_table.find(rp);
    if(pos==dir_table.end()){
	internal_error("%s: Directory %s not found in done_processing_dir", __progname, rp.c_str());
	// will not be reached.
    }
    dir_table.erase(pos);
}


int processing_dir(const string &fn)
{
    std::string rp = get_realpath8(fn);
    if(dir_table.find(rp)!=dir_table.end()){
	internal_error("%s: Attempt to add existing %s in processing_dir", __progname, rp.c_str());
    }
    dir_table.insert(rp);
    return TRUE;
}


int have_processed_dir(TCHAR *fn)
{
    return dir_table.find(get_realpath8(fn))!=dir_table.end();
}




