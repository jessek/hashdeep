// $Id$ 

/** hashlist.cpp
 * Contains the logic for performing the audit.
 * Also has implementation of the hashlist, searching, etc.
 * Formerly this code was in audit.cpp and match.cpp.
 */

#include "main.h"
#include <new>
#include <iostream>

void hashlist::hashmap::add_file(file_data_t *fi,int alg_num)
{
    if(fi->hash_hex[alg_num].size()){
	insert(std::pair<std::string,file_data_t *>(fi->hash_hex[alg_num],fi));
    }
}


/**
 * Adds a file_data_t pointer to the hashlist.
 * Does not copy the object.
 * Object will be modified from matching.
 */
void hashlist::add_fdt(file_data_t *fi)
{ 
    push_back(fi);			// retain our copy
    for(int i=0;i<NUM_ALGORITHMS;i++){	// and add for each algorithm
	hashmaps[i].add_file(fi,i); // and point to the back
    };
}

/** 
 * search for a hash with an (optional) given filename.
 * Return the first hash that matches the filename.
 * If nothing matches the filename, return the first hash that matches.
 * If a match is found, set file_number in the hash that is found.
 * Not sure I like modifying the store, but it's okay for now.
 */
file_data_t *hashlist::find_hash(hashid_t alg,const std::string &hash_hex,
				       const std::string &file_name,uint64_t file_number)
{
    std::pair<hashmap::iterator,hashmap::iterator> match = this->hashmaps[alg].equal_range(hash_hex);
    if(match.first==match.second) return 0; // nothing found
    for(hashmap::iterator it = match.first; it!=match.second; ++it){
	if((*it).second->file_name == file_name){
	    if(file_number) (*it).second->matched_file_number = file_number;
	    return (*it).second;
	}
    }
    /* No exact matches; return the first match */
    if(file_number) (*match.first).second->matched_file_number = file_number;
    return(*match.first).second;
}


/**
 * Search for the provided fdt in the hashlist and return the status of the match.
 * Match on name if possible; otherwise match on just the hash codes.
 */
hashlist::searchstatus_t hashlist::search(const file_data_hasher_t *fdht,file_data_t ** matched_) 
{
    /* Iterate through each of the hashes in the haslist until we find a match.
     */
    for (int alg = 0 ; alg < NUM_ALGORITHMS ; ++alg)  {
	/* Only search hash functions that are in use and hashes that are in the fdt */
	if (hashes[alg].inuse==0 || fdht->hash_hex[alg].size()==0){
	    continue;
	}

	/* Find the best match using find_hash */
	file_data_t *matched = find_hash((hashid_t)alg,
					       fdht->hash_hex[alg],fdht->file_name,fdht->file_number);

	if(!matched){
	    continue;			// no match
	}
	if(matched_) *matched_ = matched; // note the match

	/* Verify that all of the other hash functions for *it match fdt as well,
	 * but only for the cases when we have a hash for both the master file
	 * and the target file. */
	for(int j=0;j<NUM_ALGORITHMS;j++){
	    if(hashes[j].inuse && j!=alg
	       && fdht->hash_hex[j].size()
	       && matched->hash_hex[j].size()){
		if(fdht->hash_hex[j] != matched->hash_hex[j]){
		    /* Amazing. We found a match on one hash a a non-match on another.
		     * Call the newspapers! This is a newsorthy event.
		     */
		    return status_partial_match;
		}
	    }
	}
	/* If we got here we matched on all of the hashes.
	 * Which is to be expected.
	 * Check to see if the sizes are the same.
	 */
	if(fdht->file_bytes != matched->file_bytes){
	    /* Amazing. We found two files that have the same hash but different
	     * file sizes. This has never happened before in the history of the world.
	     * Call the newspapers!
	     */
	    return status_file_size_mismatch;
	}
	/* See if the hashes are the same but the name changed.
	 */
	if(fdht->file_name != matched->file_name){
	    return status_file_name_mismatch;
	}
	/* If we get here, then all of the hash matches for all of the algorithms have been
	 * checked and found to be equal if present.
	 */
	return status_match;
    }
    /* If we get here, nothing ever matched. Kind of sad. */
    return status_no_match;
}



/**
 * Returns the file type of a given input file.
 * fn is provided so that error messages can be printed.
 */
hashlist::hashfile_format hashlist::identify_format(class display *ocb,const std::string &fn,FILE *handle)
{
    char buf[MAX_STRING_LENGTH];

    // Find the header 
    if ((fgets(buf,MAX_STRING_LENGTH,handle)) == NULL) {
	return file_unknown;
    }

    chop_line(buf);

    if ( ! STRINGS_EQUAL(buf,HASHDEEP_HEADER_10)) {
	return file_unknown;
    }

    // Find which hashes are in this file
    if ((fgets(buf,MAX_STRING_LENGTH,handle)) == NULL) {
	return file_unknown;
    }

    chop_line(buf);

    // We don't use STRINGS_EQUAL here because we only care about
    // the first ten characters for right now. 
    if (strncasecmp("%%%% size,",buf,10))  {
	return file_unknown;
    }

    /**
     * Remember previously loaded hashes.
     */
    std::string previously_enabled_algorithms = last_enabled_algorithms; 
    
    // Skip the "%%%% size," when parsing the list of hashes 
    enable_hashing_algorithms_from_hashdeep_file(ocb,fn,buf + 10);
    

    // If the set of hashes now in use doesn't match those previously in use,
    // give a warning.
    if (previously_enabled_algorithms.size()>0
	&& previously_enabled_algorithms != last_enabled_algorithms){
	if(ocb) ocb->print_error("%s: %s: Hashes not in same format as previously loaded",__progname, fn.c_str());
    }
    return file_hashdeep_10;
}


/*
 * Examine the list of hashing algorithms in the file,
 * enable them and note their order. If the last algorithm is 'filename', ignore it.
 */

void hashlist::enable_hashing_algorithms_from_hashdeep_file(class display *ocb,const std::string &fn,std::string val)
{
    // The first position is always the file size, so we start with an 
    // the first position of one.
    uint8_t num_columns = 1;		
  
    last_enabled_algorithms = val;
    std::vector<std::string> algs = split(val,',');
    for(std::vector<std::string>::iterator it = algs.begin(); it!=algs.end(); it++){
	std::string name = *it;
	lowercase(name);
	if(name=="filename") continue;
	hashid_t id = algorithm_t::get_hashid_for_name(name);
	if(id==alg_unknown){
	    if(ocb){
		ocb->print_error("%s: %s: Badly formatted file", __progname, fn.c_str());
		ocb->try_msg();
	    }
	    exit(EXIT_FAILURE);
	}
	    
	/* Found a known algorithm */
	hashes[id].inuse = TRUE;
	hash_column[num_columns] = id;
	num_columns++;
    }
}


void hashlist::dump_hashlist()
{
    for(hashlist::const_iterator it = begin(); it!=end(); it++){
	std::cout << (*it)->file_bytes << "," << (*it)->hash_hex[alg_md5] << "," << (*it)->file_name
		  << "  matched=" << (*it)->matched_file_number << "\n";
    }
}

uint64_t hashlist::total_matched()
{
    uint64_t total = 0;
    for(hashlist::const_iterator it = begin(); it!=end(); it++){
	if( (*it)->matched_file_number > 0) total++;
    }
    return total;
}


/**
 * Loads a file of known hashes.
 * First identifies the file type, then reads the file.
 */
hashlist::loadstatus_t hashlist::load_hash_file(display *ocb,const std::string &fn)
{
    loadstatus_t status = loadstatus_ok;
    hashfile_format type;

    FILE *hl_handle = fopen(fn.c_str(),"rb");
    if (NULL == hl_handle) {
	if(ocb) ocb->print_error("%s: %s: %s", __progname, fn.c_str(), strerror(errno));
	return status_file_error;
    }
  
    type = identify_format(ocb,fn,hl_handle);
    if (file_unknown == type)  {
	if(ocb) ocb->print_error("%s: %s: Unable to identify file format", __progname, fn.c_str());
	fclose(hl_handle);
	hl_handle = 0;
	return status_unknown_filetype;
    }

    int contains_bad_lines = FALSE;
    int record_valid=0;

    // We start our counter at line number two for the two lines
    // of header we've already read
    uint64_t line_number = 2;

    /* Redo this to use std::string everywhere */
    char line[MAX_STRING_LENGTH];	// holds the line we are reading

    while (fgets(line,MAX_STRING_LENGTH,hl_handle)) {
	line_number++;			// count where we are

	// Lines starting with a pound sign are comments and can be ignored
	if ('#' == line[0]){
	    continue;
	}

	// C++ typically fails with a bad_alloc, but you can make it return null
	// http://www.cplusplus.com/reference/std/new/bad_alloc/
	// http://www.cplusplus.com/reference/std/new/nothrow/
	file_data_t *t = new (std::nothrow) file_data_t(); // C++ new fails with a bad_a
	if (NULL == t){
	    ocb->fatal_error("%s: %s: Out of memory in line %"PRIu64, 
			__progname, fn.c_str(), line_number);
	}

	chop_line(line);
	record_valid = TRUE;

	// completely rewritten to use STL strings
	std::vector<std::string> fields = split(std::string(line),',');
	for(size_t column_number=0;column_number<fields.size();column_number++){
	    // The first column should always be the file size
	    std::string word = fields[column_number];
	    if (column_number==0) {
		t->file_bytes = (uint64_t)strtoll(word.c_str(),NULL,10);
		continue;
	    }
	    if (column_number==fields.size()-1){
		t->file_name = word;
		continue;
	    }


	    // All other columns should contain a valid hash in hex
	    if ( !algorithm_t::valid_hash(hash_column[column_number],word)){
		if(ocb) ocb->print_error("%s: %s: Invalid %s hash in line %"PRIu64,__progname, fn.c_str(), 
					 hashes[hash_column[column_number]].name.c_str(),
					 line_number);
		contains_bad_lines = TRUE;
		record_valid = FALSE;
		// Break out (done = true) and then process the next line
		break;
	    }

	    // Convert the hash to a std::string and save it
	    lowercase(word);
	    t->hash_hex[hash_column[column_number]] = word;
	}
	if ( record_valid) {
	    add_fdt(t);	/* add the file to the database*/
	}
    }
    fclose(hl_handle);
    hl_handle = 0;

    if (contains_bad_lines){
	return status_contains_bad_hashes;
    }
    
    return status;
}


 /**
  * We don't use this function anymore, but it's handy to have just in case
  */
const char *hashlist::searchstatus_to_str(searchstatus_t val)
{
    switch (val) {
    case searchstatus_ok: return "ok";
    case status_match:    return "complete match";
    case status_partial_match: return "partial match";
    case status_file_size_mismatch: return "file size mismatch";
    case status_file_name_mismatch: return "file name mismatch";
    case status_no_match: return "no match"; 
	
    default:
	return "unknown";
    }      
}




