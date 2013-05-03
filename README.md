This is md5deep, a set of cross-platform tools to computer hashes, or
message digests, for any number of files while optionally recursively
digging through the directory structure.  It can also take a list of known
hashes and display the filenames of input files whose hashes either do or
do not match any of the known hashes. This version supports MD5, SHA-1,
SHA-256, Tiger, and Whirlpool hashes.

See the file [NEWS](NEWS) for a list of changes between releases.

See the file [COPYING](COPYING) for information about the licensing for this program.

See the file [INSTALL](INSTALL) for (generic) compilation and installation
instructions. Here's the short version that should just work in many cases:

```shell
sh bootstrap.sh # runs autoconf, automake
./configure
make
make install
```

Note that you must be normally root to install to the default location.
The sudo command is helpful for doing so. You can specify an alternate
installation location using the --prefix option to the configure script.
For example, to install to /home/foo/bin, use:

>$ ./configure --prefix=/home/foo

There is complete documentation on how to use the program on the
project's homepage, [https://github.com/jessek/hashdeep](https://github.com/jessek/hashdeep)

## md5deep vs. hashdeep

For historical reasons, the program has different options and features
when run with the names "hashdeep" and "md5deep."

hashdeep has a feature called "audit" which:
> \* Can also use a list of known hashes to audit a set of FILES. Errors
>   are reported to standard error. If no FILES are specified, reads from
>   standard input.
>
> -a Audit mode. Each input file is compared against the set of knowns. An
>    audit is said to pass if each input file is matched against exactly
>    one file in set of knowns. Any collisions, new files, or missing files
>    will make the audit fail. Using this flag alone produces a message,
>    either "Audit passed" or "Audit Failed".
>
>    -v - prints the number of files in each category
>    -v -v = prints all discrepancies
>    -v -v -v = prints the results for every file examined and every known file.
>
> -k <file> - The -k option must be used to load the audit file

To perform an audit:
>  hashdeep -r dir  > /tmp/auditfile            # Generate the audit file
>  hashdeep -a k /tmp/auditfile -r dir          # test the audit

Notice that the audit is performed with a standard hashdeep output
file. (Internally, the audit is computed as part of the hashing process.)

## Unicode Issues
POSIX-based modern computer systems consider filenames to be a
sequence of bytes that are rendered as the application wishes. This
means that filenames typically contain ASCII but can contain UTF-8,
UTF-16, latin1, or even invalid Unicode codings.

Windows-based systems have one set of API calls for ASCII-based
filenames and another set for filenames encoded as UCS-2, which
"produces a fixed-length format by simply using the code point as the
16-bit code unit and produces exactly the same result as UTF-16 for
63,488 code points in the range 0-0xFFFF" according to [wikipedia]
(http://en.wikipedia.org/wiki/UTF-16/UCS-2). But wikipedia disputes the
factual accuracy of this statement on the talk page. it's pretty clear
that nobody is entirely sure that Windows actually does, and Windows
itself may not be consistent.

Version 3 of this program addressed this issue by using the TCHAR
variable to hold filenames on Windowa dn by refusing to print them,
priting a "?" instead. Version 4 of this program translates TCHAR
strings to std::string strings at the soonest opportunity using the
[Windows function WideCharToMultiByte]
(http://msdn.microsoft.com/en-us/library/dd374130%28v=vs.85%29.aspx). Flags
have been added escape Unicode when it is printed.

There is no way (apparently) on Windows to open a UTF-8 filename; it needs to be
converted back to a multi-byte filename with MultiByteToWideChar.

Fortunately, we never really need to convert back.

Notice that on Windows the files hashed can have unicode characters
but the file with the hashes must have an ASCII name.

COMPILING FOR WINDOWS:
> -D_UNICODE causes TCHAR to be defined as 'wchar_t'.

COMPILING FOR POSIX:
> -D_UNICODE is not defined, causing TCHAR to be defined as 'char'.

Previously, win32 functions were controlled with #ifdef statements, like this:

```C
#ifdef _WIN32
  _wfullpath(d_name,fn,PATH_MAX);
#else
  if (NULL == realpath(fn,d_name))
    return TRUE;
#endif
```

There was also a file called tchar-local.h which actually changed the semantics
of functions on different platforms, with things like this:

```C
   #define  _tcsncpy   strncpy
   #define  _tstat_t   struct stat
```

This made the code very difficult to maintain.

With the 4.0 rewrite, we have changed this code with C++ functions that return
objects were possible and avoid the use of #defines that so that on _WIN32 systems
the function realpath() gets defined prior to its use, and the mainline code
lacks the realpath() function. You can see this in cycles.cpp:

```C
/* Return the canonicalized absolute pathname in UTF-8 on Windows and POSIX systems */
std::string get_realpath(const TCHAR *fn)
{
#ifdef _WIN32
    /*
     * expand a relative path to the full path.
     * http://msdn.microsoft.com/en-us/library/506720ff(v=vs.80).aspx
     */
    TCHAR absPath[PATH_MAX];
    if(_fullpath(absPath,fn,PAT_HMAX)==0) return "";
    return tchar_to_utf8(absPath);
#else
    char resolved_name[PATH_MAX];	//
    if(realpath(fn,resolved_name)==0) return "";
    return string(resolved_name);
#endif
}
```

You can install mingw and then simply configure with something like this:
>$ export PATH=$PATH:/usr/local/i386-mingw32-4.3.0/bin
>$ ./configure --host=i386-mingw32


## Hash Algorithm References

The MD5 algorithm is defined in RFC 1321:
http://www.ietf.org/rfc/rfc1321.txt

The SHA1 algorithm is defined in FIPS 180-1:
http://www.itl.nist.gov/fipspubs/fip180-1.htm

The SHA256 algorithm is defined FIPS 180-2:
http://csrc.nist.gov/publications/fips/fips180-2/fips180-2.pdf

The Tiger algorithm is defined at:
http://www.cs.technion.ac.il/~biham/Reports/Tiger/

The Whirlpool algorithm is defined at:
http://planeta.terra.com.br/informatica/paulobarreto/WhirlpoolPage.html

## Theory of Operation

* main.cpp
  * sets up the system
* dig.cpp
  * iterates through the individual directories
  * calls hash_file() in hash.cpp for each file to hash
* hash.cpp
  * performs the hashing of each file
* display.cpp
  * stores/displays the results
