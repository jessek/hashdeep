// MD5DEEP - winpe.cpp
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


bool has_executable_extension(const tstring &fn)
{
  size_t last_period = fn.find_last_of(_TEXT("."));

  // If there is no extension, we're done.
  if (std::string::npos == last_period)
    return false;

  // Get the file extension and convert it to lowercase
  tstring extension = fn.substr(last_period);
  std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

#define CHECK_EXEC(A)  if (A == extension) return true;
  CHECK_EXEC(_TEXT(".exe"));
  CHECK_EXEC(_TEXT(".dll"));
  CHECK_EXEC(_TEXT(".com"));
  CHECK_EXEC(_TEXT(".sys"));
  CHECK_EXEC(_TEXT(".cpl"));
  CHECK_EXEC(_TEXT(".hxs"));
  CHECK_EXEC(_TEXT(".hxi"));
  CHECK_EXEC(_TEXT(".olb"));
  CHECK_EXEC(_TEXT(".rll"));
  CHECK_EXEC(_TEXT(".mui"));
  CHECK_EXEC(_TEXT(".tlb"));

  return false;
}


bool is_pe_file(const unsigned char * buffer, size_t size)
{
  // We need at least 0x40 bytes to hold an IMAGE_DOS_HEADER
  // and the signature of a PE header.
  if (NULL == buffer or size < 0x40)
    return false;

  // Is the MZ header's signature 'MZ'?
  uint16_t mz_header = buffer[0] | (buffer[1] << 8);
  if (0x5a4d != mz_header)
    return false;

  // Find the PE header. It's the e_lfanew field in the IMAGE_DOS_HEADER
  // structure, which is at offset 0x3c.
  // This line is equivalent to:
  //    uint16_t pe_offset = *(uint16_t *)(buffer + 0x3c);
  // but is not affected by the endianness of the system.
  // This value should be a uint16_t according to the IMAGE_DOS_HEADER
  // but that merits us a compiler warning. size_t *should* be wider than
  // 16 bits on your platform. Or else you need a better platform. Just sayin'.
  size_t pe_offset = buffer[0x3c] | (buffer[0x3d] << 8);

  // Do we have enough data to do this check?
  if (pe_offset + 4 > size)
    return false;

  // Is the PE header's signature 'PE  '? The PE signature should begin
  // at the location specified by the PE offset in the DOS header
  // This line is equivalent to:
  //    uint32_t signature = *(uint32_t *)(buffer + pe_offset);
  // but is not affected by the endianness of the system.
  const unsigned char * tmp = buffer+pe_offset;
  uint32_t signature=tmp[0] | (tmp[1] << 8) | (tmp[2] << 16) | (tmp[3] << 24);
  if (signature != 0x4550)
    return false;

  return true;
}
