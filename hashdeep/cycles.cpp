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
 *             in a set of tstring strings.
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



