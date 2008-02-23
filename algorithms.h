
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
   How to add more algorithms to the md5deep suite

   1. Create a new ifdef statement below to include your header file.
      You will need to choose a compiler define to use for your
      algorithm. For example, if you're adding the cows algorithm, 
      you could use:

      #ifdef COWS
      #include "cows.h"
      #endif

   2. Create the header file for your algorithm. This file will define
      the generic functions used in hash.c with the specific calls to
      your algorithm. See md5.h for more details.

   3. Add the source code for the algorithm to the source code directory.
   
   4. Add commands to the Makefile to compile your program. To wit:

       a. Add the program name to the ALL_GOALS and COMMA_GOALS variables.

       b. Add the source and header files for your algorithm to the SRC
          and HEADER_FILES variables, respectively.

       c. Create a set of rules to create files-[alg].o and  hash-[alg].o.
          For COWS, this might look like:

          files-cows.o: files.c
	    $(CC) -c $< -o $@

       d. Create the rules to assemble the package. For cows, this would be:

          COWSOBJ = files-cows.o hash-cows.o cows.o
	  cowsdeep: CC += -DCOWS
	  cowsdeep: $(OBJ) $(COWSOBJ)
	     $(CC) $(OBJ) $(COWSOBJ) -o $@$(SUFFIX) $(LINK_OPT)

       e. Add a line to the install directive to create a man page. For cows:
           
          ln -fs md5deep.1 $(MAN)/cowsdeep.1


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

#ifdef WHIRLPOOL
#include "whirlpool.h"
#endif

#ifdef TIGER
#include "tiger.h"
#endif
