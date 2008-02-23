
/* MD5DEEP - algorithms.h
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

/* --------------------------------------------------------------------
   How to add more algorithms to md5deep

   1. Create a new ifdef statement below to include your header file.
      You will need to choose a compiler define to use for your
      algorithm. For example, if you're adding SHA-256, you might want
      to add:

      #ifdef SHA256
      #include "sha256.h"
      #endif

   2. Create the header file for your algorithm. This file will define
      the generic functions used in hash.c with the specific calls to
      your algorithm. See md5.h for more details.

   3. Add the source code for the algorithm to the source code directory.

   4. Modify the Makefile to include the calls to compile the program
      using your algorithm. Usually this means:
       a. Add the progname name to the goals directive.
       b. Create a set of rules to create files-[alg].o, hash-[alg].o,
          hashTable-[alg].o and [alg].o. For SHA-256, this might look like:

          files-sha256.o: files.c
	    $(CC) -DSHA256 -c files.c -o files-sha256.c

   5. Add your algorithm to the documentation! 
   -------------------------------------------------------------------- */

#ifdef MD5
#include "md5.h"
#endif

#ifdef SHA1
#include "sha1.h"
#endif

#ifdef SHA256
#include "sha256.h"
#endif
