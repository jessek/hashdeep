/*
 * Simson's XML output class.
 * Ideally include this AFTER your config file with the HAVE statements.
 *
 * Pubic Domain.
 */

#ifndef _XML_H_
#define _XML_H_

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <inttypes.h>
#include <string>
#include <stack>
#include <set>

#ifdef HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

// pwd.h is password entry
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#ifdef HAVE_LIBAFFLIB
#include <afflib/afflib.h>
#endif

#ifdef HAVE_LIBEWF
#include <libewf.h>
#endif

#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif

#ifdef HAVE_LIBAFFLIB
#include <afflib/afflib.h>
#endif

#ifdef HAVE_SYS_RUSAGE_H
#include <sys/rusage.h>
#endif

#ifdef __cplusplus
class XML {
    std::string outfilename;
    FILE  *out;				// where it is being written
    std::set<std::string> tags;			// XML tags
    void  write_doctype(FILE *out);
    void  write_dtd(FILE *out);
    bool  make_dtd;
    std::string  tempfilename;
    void  verify_tag(std::string tag);
    std::stack<std::string>tag_stack;
    void spaces();			// print spaces corresponding to tag stack
    std::string tempfile_template;
    struct timeval t0;

public:
    void make_indent(){spaces();}
    static std::string make_command_line(int argc,char * const *argv){
	std::string command_line;
	for(int i=0;i<argc;i++){
	    if(i>0) command_line.push_back(' ');
	    command_line.append(argv[i]);
	}
	return command_line;
    }

    XML();			
    XML(FILE *out);

    void set_outFILE(FILE *out);	  // writes to this FILE without a DTD
    void set_outfilename(std::string outfilename);     // writes to this outfile with a DTD (needs a temp file)
    void set_makeDTD(bool flag);		 // should we write the DTD?
    void set_tempfile_template(std::string temp);

    static std::string xmlescape(const std::string &xml);
    static std::string xmlstrip(const std::string &xml);
    
    void open();			// opens the output file
    void close();			// writes the output to the file
    void writexml(const std::string &xml); // writes formatted xml with indentation
    void tagout(const std::string &tag,const std::string &attribute);
    void xmlout(const std::string &tag,const std::string &value, const std::string &attribute,const bool escape_value);
    void xmlout(const std::string &tag,const std::string &value){ xmlout(tag,value,"",true); }
    void xmlout(const std::string &tag,const int value){ xmlprintf(tag,"","%d",value); }
    void xmloutl(const std::string &tag,const long value){ xmlprintf(tag,"","%ld",value); }
    void xmlout(const std::string &tag,const int64_t value){ xmlprintf(tag,"","%"PRId64,value); }
    void xmlout(const std::string &tag,const double value){ xmlprintf(tag,"","%f",value); }
    void xmlout(const std::string &tag,const struct timeval &ts){
	xmlprintf(tag,"","%d.%06d",(int)ts.tv_sec, (int)ts.tv_usec);
    }

    void xmlprintf(const std::string &tag,const std::string &attribute,
		   const char *fmt,...);
    void push(const std::string &tag,const std::string &attribute);
    void push(const std::string &tag) {push(tag,"");}
    void puts(const std::string &pdata);	// writes a std::string as parsed data
    void printf(const char *fmt,...);	// writes a std::string as parsed data
    void pop();	// close the tag
    void comment(const std::string &comment);

/* These support Digital Forensics XML and require certain variables to be defined */
    void add_DFXML_build_environment(){
	/* __DATE__ formats as: Apr 30 2011 */
	struct tm tm;
	memset(&tm,0,sizeof(tm));
	push("build_environment");
#ifdef __GNUC__
	xmlprintf("compiler","","GCC %d.%d",__GNUC__, __GNUC_MINOR__);
#endif
#if defined(__DATE__) && defined(__TIME__) && defined(HAVE_STRPTIME)
	if(strptime(__DATE__,"%b %d %Y",&tm)){
	    char buf[64];
	    snprintf(buf,sizeof(buf),"%4d-%02d-%02dT%s",tm.tm_year+1900,tm.tm_mon+1,tm.tm_mday,__TIME__);
	    xmlout("compilation_date",buf);
	}
#endif
#ifdef HAVE_LIBTSK3
	xmlout("library", "", std::string("name=\"tsk\" version=\"") + tsk_version_get_str() + "\"",false);
#endif
#ifdef HAVE_LIBAFFLIB
	xmlout("library", "", std::string("name=\"afflib\" version=\"") + af_version() +"\"",false);
#endif
#ifdef HAVE_LIBEWF
	xmlout("library", "", std::string("name=\"libewf\" version=\"") + libewf_get_version() + "\"",false);
#endif
	pop();
    }

    void add_DFXML_execution_environment(const std::string &command_line){
	push("execution_environment");
#ifdef HAVE_SYS_UTSNAME_H
	struct utsname name;
	if(uname(&name)==0){
	    xmlout("os_sysname",name.sysname);
	    xmlout("os_release",name.release);
	    xmlout("os_version",name.version);
	    xmlout("host",name.nodename);
	    xmlout("arch",name.machine);
	}
#else
#ifdef UNAMES
	xmlout("os_sysname",UNAMES,"",false);
#endif
#ifdef HAVE_GETHOSTNAME
	{
	    char hostname[1024];
	    if(gethostname(hostname,sizeof(hostname))==0){
		xmlout("host",hostname);
	    }
	}
#endif
#endif	
	xmlout("command_line", command_line); // quote it!
#ifdef HAVE_GETUID
	xmlprintf("uid","","%d",getuid());
#ifdef HAVE_GETPWUID
	xmlout("username",getpwuid(getuid())->pw_name);
#endif
#endif

	time_t t = time(0);
	char buf[32];
	strftime(buf,sizeof(buf),"%Y-%m-%dT%H:%M:%SZ",gmtime(&t));
	xmlout("start_time",buf);
	pop();			// <execution_environment>
    }
    void add_DFXML_creator(const std::string &program,const std::string &version,const std::string &command_line){
	push("creator","version='1.0'");
	xmlout("program",program);
	xmlout("version",version);
	add_DFXML_build_environment();
	add_DFXML_execution_environment(command_line);
	pop();			// creator
    }

    void add_rusage(){
#ifdef HAVE_GETRUSAGE
	struct rusage ru;
	memset(&ru,0,sizeof(ru));
	if(getrusage(RUSAGE_SELF,&ru)==0){
	    push("rusage");
	    xmlout("utime",ru.ru_utime);
	    xmlout("stime",ru.ru_stime);
	    xmloutl("maxrss",(long)ru.ru_maxrss);
	    xmloutl("minflt",(long)ru.ru_minflt);
	    xmloutl("majflt",(long)ru.ru_majflt);
	    xmloutl("nswap",(long)ru.ru_nswap);
	    xmloutl("inblock",(long)ru.ru_inblock);
	    xmloutl("oublock",(long)ru.ru_oublock);
	    {
		struct timeval t1;
		gettimeofday(&t1,0);
		struct timeval t;

		t.tv_sec = t1.tv_sec - t0.tv_sec;
		if(t1.tv_usec > t0.tv_usec){
		    t.tv_usec = t1.tv_usec - t0.tv_usec;
		} else {
		    t.tv_sec--;
		    t.tv_usec = (t1.tv_usec+1000000) - t0.tv_usec;
		}
		xmlout("clocktime",t);
	    }
	    pop();
	}
#endif
    }
};
#endif

#endif
