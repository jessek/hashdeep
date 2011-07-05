/*
 * C++ implementation of a wchar_t string.
 * Implemented as a C++ vector, since my attempts to implement as a wchar_t array failed.
 * It works like the regular string, except that it works with wide chars.
 * Notice that .c_str() returns a wchar_t * pointer.
 *
 * You will find a nice discussion about allocating arrays in C++ at:
 * http://www.fredosaurus.com/notes-cpp/newdelete/50dynamalloc.html
 *
 * Simson L. Garfinkel, 2011
 * Public Domain (a work of the US Government).
 */
#include <vector>
class wstring : public std::vector<wchar_t>{
    mutable wchar_t *cstr;			// a null-terminated string
public:
    static const ssize_t npos=-1;
    wstring():cstr(0){}
    wstring(const wchar_t *buf):cstr(0){
	/* this is pretty inefficient */
	while(*buf){
	    push_back(*buf);
	    buf++;
	}
    }
    wstring(const wchar_t *buf,size_t len):cstr(0){
	/* This is also pretty inefficient */
	while(len){
	    push_back(*buf);
	    buf++;
	    len--;
	}
    }
    wstring(const char *str):cstr(0){ /* legacy C-style strings */
	while(*str){
	    push_back(*str);
	    str++;
	}
    }
    ~wstring(){
	invalidate_cstr();
    }

    void invalidate_cstr() const {
	if(cstr){
	    delete[] cstr;
	    cstr = 0;
	}
    }

    /* utf16() and c_str() return a null-terminated string.
     * This needs to be separately tracked and freed as necessary.
     */
    const wchar_t *utf16() const{		
	if(cstr) return cstr;
	cstr = new wchar_t[size()+1];
	/* http://www.parashift.com/c++-faq-lite/containers.html#faq-34.2 */
	memmove(cstr,&(*this)[0],size()*sizeof(wchar_t));
	cstr[size()] = 0;		// null-terminate
	return cstr;
    }
    const wchar_t *c_str() const{		// wide c strings *are* utf16
	return utf16();
    }
    ssize_t find(wchar_t ch,size_t start=0) const{
	for(size_t i=start;i<size();i++){
	    if((*this)[i]==ch) return i;
	}
	return npos;
    }
    ssize_t rfind(wchar_t ch,size_t start=0) const{
	if(size()==0) return npos;
	if(start==0 || start>size()) start=size()-1;
	for(size_t i=start;i>0;i--){
	    if((*this)[i]==ch) return i;
	    if(i==0) return npos;	// not found
	}
	return npos;			// should never reach here
    }
    ssize_t find(const wstring &s2,ssize_t start=0) const{
	if(s2.size() > size()) return npos;
	for(size_t i=start;i<size()-s2.size();i++){
	    for(size_t j=0;j<s2.size();j++){
		std::cerr << "i="<<i<<" j="<<j<<"\n";
		if((*this)[i+j]!=s2[j]) break;
		if(j==s2.size()-1) return i;	// found it
	    }
	}
	return npos;
    }
    ssize_t rfind(const wstring &s2) const{
	if((s2.size() > this->size()) || (this->size()==0)) return npos;
	for(size_t i=size()-s2.size()-1;i>=0;i--){
	    for(size_t j=0;j<s2.size();j++){
		if((*this)[i+j]!=s2[j]) break;
		if(j==size()-1) return i;	// found it
	    }
	    if(i==0)return npos;	// got to 0 and couldn't find the end
	}
	return npos;			// should never get here.
    }
#if 0
    /* Be really cool if I could get this to work. */
    void erase(size_t  start,size_t count){
	std::vector<wchar_t>::iterator i1 = begin();
	erase(i1+start,i1+start+count);
    }
    void erase(size_t start){
	erase(this->begin()+start);
    }
#endif
    wstring substr(size_t pos,size_t len) const{
	if(pos>size()) return wstring();
	if(pos+len>size()) len=size()-pos;
	return wstring(utf16()+pos,len);
    }
    void append(const wstring &s2){    /* Append a string */
	invalidate_cstr();
	for(wstring::const_iterator it = s2.begin(); it!=s2.end(); it++){
	    push_back(*it);
	}
    }
    void append(const TCHAR *s2){	// append a TCHAR array
	invalidate_cstr();
	while(*s2){
	    push_back(*s2);
	    s2++;
	}
    }
    void append(const char *s2){	// append a cstring
	invalidate_cstr();
	while(*s2){
	    push_back(*s2);
	    s2++;
	}
    }

    /* Operators follow */
    wstring & operator+=(const wstring &s2) { // append a wstring 
	this->append(s2);
	return *this;
    }
    wstring & operator+=(const char *str) { // append a c string
	this->append(str);
	return *this;
    }

    /* Concatenate a wstring and return a new strong */
    wstring operator+(const wstring &s2) const {
	wstring s1(*this);
	s1.append(s2);
	return s1;
    }
    /* Concatenate a TCHAR * string */
    wstring operator+(const TCHAR *s2) const {
	wstring s1(*this);
	s1.append(s2);
	return s1;
    }
    wstring operator+(TCHAR c2) const {
	wstring s1(*this);
	s1.push_back(c2);
	return s1;
    }
    bool operator<(const wstring &s2) const {
	for(size_t i=0;i<size() && i<s2.size();i++){
	    if((*this)[i] < s2[i]) return true;
	}
	if(size() < s2.size()) return true;
	return false;
    }
    bool operator>(const wstring &s2) const {
	for(size_t i=0;i<size() && i<s2.size();i++){
	    if((*this)[i] > s2[i]) return true;
	}
	if(size() > s2.size()) return true;
	return false;
    }
    bool operator==(const wstring &s2) const {
	if(size()!=s2.size()) return false;
	for(size_t i=0;i<size();i++){
	    if((*this)[i] != s2[i]) return false;
	}
	if(size() == s2.size()) return true;
	return true;
    }
};


