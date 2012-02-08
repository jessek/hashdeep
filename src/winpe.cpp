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


// These structures are already defined on Win32

#ifndef _WIN32

typedef struct _IMAGE_SECTION_HEADER {
  /* 000 */ unsigned char                Name[8];
  union {
    /* 008 */ uint32_t                   PhysicalAddress;
    /* 008 */ uint32_t                   VirtualSize;
  } Misc;
  /* 00c */ uint32_t                     VirtualAddress;
  /* 010 */ uint32_t                     SizeOfRawData;
  /* 014 */ uint32_t                     PointerToRawData;
  /* 018 */ uint32_t                     PointerToRelocations;
  /* 01c */ uint32_t                     PointerToLinenumbers;
  /* 020 */ uint16_t                     NumberOfRelocations;
  /* 022 */ uint16_t                     NumberOfLinenumbers;
  /* 024 */ uint32_t                     Characteristics;
  /* Size 028 */
} IMAGE_SECTION_HEADER;


typedef struct _IMAGE_DATA_DIRECTORY {
  /* 000 */ uint32_t                     VirtualAddress;
  /* 004 */ uint32_t                     Size;
  /* Size 008 */
} IMAGE_DATA_DIRECTORY;


typedef struct _IMAGE_FILE_HEADER {
  /* 000 */ uint16_t                     Machine;
  /* 002 */ uint16_t                     NumberOfSections;
  /* 004 */ uint32_t                     TimeDateStamp;
  /* 008 */ uint32_t                     PointerToSymbolTable;
  /* 00c */ uint32_t                     NumberOfSymbols;
  /* 010 */ uint16_t                     SizeOfOptionalHeader;
  /* 012 */ uint16_t                     Characteristics;
  /* Size 014 */
} IMAGE_FILE_HEADER;

typedef struct _IMAGE_OPTIONAL_HEADER {
  /* 000 */  uint16_t                    Magic;
  /* 002 */  unsigned char               MajorLinkerVersion;
  /* 003 */  unsigned char               MinorLinkerVersion;
  /* 004 */  uint32_t                    SizeOfCode;
  /* 008 */  uint32_t                    SizeOfInitializedData;
  /* 00c */  uint32_t                    SizeOfUninitializedData;
  /* 010 */  uint32_t                    AddressOfEntryPoint;
  /* 014 */  uint32_t                    BaseOfCode;
  /* 018 */  uint32_t                    BaseOfData;
  /* 01c */  uint32_t                    ImageBase;
  /* 020 */  uint32_t                    SectionAlignment;
  /* 024 */  uint32_t                    FileAlignment;
  /* 028 */  uint16_t                    MajorOperatingSystemVersion;
  /* 02a */  uint16_t                    MinorOperatingSystemVersion;
  /* 02c */  uint16_t                    MajorImageVersion;
  /* 02e */  uint16_t                    MinorImageVersion;
  /* 030 */  uint16_t                    MajorSubsystemVersion;
  /* 032 */  uint16_t                    MinorSubsystemVersion;
  /* 034 */  uint32_t                    Win32VersionValue;
  /* 038 */  uint32_t                    SizeOfImage;
  /* 03c */  uint32_t                    SizeOfHeaders;
  /* 040 */  uint32_t                    CheckSum;
  /* 044 */  uint16_t                    Subsystem;
  /* 046 */  uint16_t                    DllCharacteristics;
  /* 048 */  uint32_t                    SizeOfStackReserve;
  /* 04c */  uint32_t                    SizeOfStackCommit;
  /* 050 */  uint32_t                    SizeOfHeapReserve;
  /* 054 */  uint32_t                    SizeOfHeapCommit;
  /* 058 */  uint32_t                    LoaderFlags;
  /* 060 */  IMAGE_DATA_DIRECTORY        DataDirectory[16];
  /* Size 0e0 */
} IMAGE_OPTIONAL_HEADER;


typedef struct _IMAGE_NT_HEADERS {
  /* 000 */ uint32_t                     Signature;
  /* 004 */ IMAGE_FILE_HEADER            FileHeader;
  /* 018 */ IMAGE_OPTIONAL_HEADER        OptionalHeader;
} IMAGE_NT_HEADERS;


#endif    // ifndef _WIN32


bool is_pe_file(const unsigned char * buffer, size_t size)
{
  if (NULL == buffer or size < 2)
    return false;

  uint16_t mz_header = *(uint16_t *)buffer;  

    // RBF - Do we need to support big endian systems?
#ifdef WORDS_BIGENDIAN
  uint16_t tmp = mz_header;
  BYTES_SWAP16(tmp);
  mz_header = tmp;
#endif

  // Is the MZ header's signature 'MZ'?
  if (0x5a4d != mz_header)
    return false;

  // Find the PE header. It's the e_lfanew field in the IMAGE_DOS_HEADER
  // structure, which is at offset 0x3c.
  uint16_t pe_offset = *(uint16_t *)(buffer + 0x3c);

  // Do we have enough data to do this check?
  if (pe_offset + 4 > size)
    return false;

  IMAGE_NT_HEADERS h = *(IMAGE_NT_HEADERS *)(buffer + pe_offset);
  uint32_t signature = h.Signature;

  // RBF - Do we need to support big endian systems?
#ifdef WORDS_BIGENDIAN
  BYTES_SWAP32(signature);
#endif

  // Is the PE header's signature 'PE'?
  if (signature != 0x4550)
    return false;

  return true;
}
