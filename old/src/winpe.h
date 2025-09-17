// MD5DEEP - winpe.h
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
// $Id$
//

#include "main.h"

#define PETEST_BUFFER_SIZE 8192

/// Returns true if 'fn' ends with an extension which is normally executable on Microsoft Windows
bool has_executable_extension(const tstring &fn);

/// Returns true if the data in buffer is the start of a PE executable
///
/// @param buffer The buffer to test
/// @param size Size of the buffer in bytes
bool is_pe_file(const unsigned char * buffer, size_t size);
