/**
 * implementation for C++ XML generation class
 *
 * The software provided here is released by the Naval Postgraduate
 * School, an agency of the U.S. Department of Navy.  The software
 * bears no warranty, either expressed or implied. NPS does not assume
 * legal liability nor responsibility for a User's use of the software
 * or the results of such use.
 *
 * Please note that within the United States, copyright protection,
 * under Section 105 of the United States Code, Title 17, is not
 * available for any work of the United States Government and/or for
 * any works created by United States Government employees. User
 * acknowledges that this software contains work which was created by
 * NPS government employees and is therefore in the public domain and
 * not subject to copyright.
 */



#include "xml.h"

using namespace std;

#include <iostream>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <sys/param.h>
#include <assert.h>
#include <fcntl.h>


// Implementation of mkstemp for windows found on pan-devel mailing
// list archive
// @http://www.mail-archive.com/pan-devel@nongnu.org/msg00294.html
#if defined(__MINGW_H)
#ifndef _S_IREAD
#define _S_IREAD 256
#endif

#ifndef _S_IWRITE
#define _S_IWRITE 128
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifndef _O_SHORT_LIVED
#define _O_SHORT_LIVED 0
#endif

extern "C"
int mkstemp(char *tmpl)
{
  int ret=-1;
  mktemp(tmpl); ret=open(tmpl,O_RDWR|O_BINARY|O_CREAT|O_EXCL|_O_SHORT_LIVED, _S_IREAD|_S_IWRITE);
  return ret;
}

#endif


static const char *cstr(const string &str){
    return str.c_str();
}

static string xml_lt("&lt;");
static string xml_gt("&gt;");
static string xml_am("&amp;");
static string xml_ap("&apos;");
static string xml_qu("&quot;");

string xml::xmlescape(const string &xml)
{
    string ret;
    for(string::const_iterator i = xml.begin(); i!=xml.end(); i++){
	switch(*i){
	case '>':  ret += xml_gt; break;
	case '<':  ret += xml_lt; break;
	case '&':  ret += xml_am; break;
	case '\'': ret += xml_ap; break;
	case '"':  ret += xml_qu; break;
	case '\000': break;		// remove nulls
	default:
	    ret += *i;
	}
    }
    return ret;
}

/**
 * Strip an XML string as necessary for a tag name.
 */

string xml::xmlstrip(const string &xml)
{
    string ret;
    for(string::const_iterator i = xml.begin(); i!=xml.end(); i++){
	if(isprint(*i) && !strchr("<>\r\n&'\"",*i)){
	    ret += isspace(*i) ? '_' : tolower(*i);
	}
    }
    return ret;
}

/****************************************************************/

xml::xml()
{
    out = 0;
    make_dtd = false;
    tempfile_template = "/tmp/xml_XXXXXXXX"; // a reasonable default
    gettimeofday(&t0,0);
}

void xml::set_tempfile_template(std::string temp)
{
    tempfile_template = temp;
}


void xml::set_outFILE(FILE *out_)
{
    out = out_;
}

void xml::set_outfilename(string outfilename_)
{
    outfilename = outfilename_;
    tempfile_template = outfilename_ + "_tmp_XXXXXXXX"; // a better default
}

void xml::set_makeDTD(bool flag)
{
    make_dtd = flag;
    if(make_dtd){			// need to write to a temp file
	char buf[1024];
	memset(buf,0,sizeof(buf));
	strcpy(buf,tempfile_template.c_str());
	int fd = mkstemp(buf);
	if(fd<0){
	    perror(buf);
	    exit(1);
	}
	tempfilename = buf;  
	out = fdopen(fd,"r+");
	if(!out){
	    perror("fdopen");
	    exit(1);
	}
    }
}

static const char *xml_header = "<?xml version='1.0' encoding='UTF-8'?>\n";

void xml::open()
{
    if(out==0) out = fopen(cstr(outfilename),"w");
    fputs(xml_header,out);		// write out the XML header
}

void xml::close()
{
    if(make_dtd){
	/* If we are making the DTD, then we have been writing to a temp file.
	 * Open the real file, write the DTD, and copy over the tempfile.
	 */
	FILE *rf = fopen(cstr(outfilename),"w");
	if(!rf){
	    perror(cstr(outfilename));
	    fprintf(stderr,"%s will not be deleted\n",tempfilename.c_str());
	    exit(1);
	}
	fseek(out,0L,SEEK_SET);
	char buf[65536];
	if(fgets(buf,sizeof(buf),out)){
	    fputs(buf,rf);		// copy over first line --- the XML header
	}
	write_dtd(rf);			// write the DTD
	while(!feof(out)){		// copy the rest
	    int r = fread(buf,1,sizeof(buf),out);
	    if(r<=0) break;
	    int r2 = fwrite(buf,1,r,rf);
	    if(r2<0){
		fprintf(stderr,"Cannot write to %s\n",cstr(outfilename));
		fprintf(stderr,"%s will not be deleted\n",tempfilename.c_str());
		exit(1);
	    }
	}
	fclose(rf);
	unlink(tempfilename.c_str());
    }
    fclose(out);
}

void xml::write_dtd(FILE *f)
{
    fprintf(f,"<!DOCTYPE fiwalk\n");
    fprintf(f,"[\n");
    for(set<string>::const_iterator it = tags.begin(); it != tags.end(); it++){
	const char *s = (*it).c_str();
	fprintf(f,"<!ELEMENT %s ANY >\n",s);
    }
    fprintf(f,"<!ATTLIST volume startsector CDATA #IMPLIED>\n");
    fprintf(f,"<!ATTLIST run start CDATA #IMPLIED>\n");
    fprintf(f,"<!ATTLIST run len CDATA #IMPLIED>\n");
    fprintf(f,"]>\n");
}

/**
 * make sure that a tag is valid and, if so, add it to the list of tags we use
 */
void xml::verify_tag(string tag)
{
    if(tag[0]=='/') tag = tag.substr(1);
    if(tag.find(" ") != string::npos){
	cerr << "tag '" << tag << "' contains space. Cannot continue.\n";
	exit(1);
    }
    tags.insert(tag);
}

void xml::puts(const string &v)
{
    fputs(v.c_str(),out);
}

void xml::spaces()
{
    string sp;
    for(int i=tag_stack.size();i>0;i--){
	sp.push_back(' ');
	sp.push_back(' ');
    }
    puts(sp);
}

void xml::tagout(const string &tag,const string &attribute)
{
    verify_tag(tag);
    fprintf(out,"<%s%s%s>",cstr(tag),attribute.size()>0 ? " " : "",cstr(attribute));
}

void xml::xmlout(const string &tag,const string &value,const string &attribute,bool escape_value)
{
    spaces();
    tagout(tag,attribute);
    if(escape_value) fputs(cstr(xmlescape(value)),out);
    else fputs(cstr(value),out);
    tagout("/"+tag,"");
    fputc('\n',out);
}


void xml::xmlprintf(const std::string &tag,const std::string &attribute, const char *fmt,...)
{
    spaces();
    tagout(tag,attribute);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(out,fmt,ap);
    va_end(ap);
    tagout("/"+tag,"");
    fputc('\n',out);
}

void xml::push(const string &tag,const string &attribute)
{
    spaces();
    tag_stack.push(tag);
    tagout(tag,attribute);
    fputc('\n',out);
}

void xml::pop()
{
    assert(tag_stack.size()>0);
    string tag = tag_stack.top();
    tag_stack.pop();
    spaces();
    tagout("/"+tag,"");
    fputc('\n',out);
}

void xml::printf(const char *format,...)
{
    va_list ap;
    va_start(ap, format);
    vfprintf(out,format,ap);
    va_end(ap);
}

void xml::comment(const string &comment)
{
    fputs("<!-- ",out);
    fputs(comment.c_str(),out);
    fputs(" -->\n",out);
}


