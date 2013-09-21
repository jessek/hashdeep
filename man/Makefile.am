ALL_GOALS={md5,sha1,sha256,sha3,tiger,whirlpool,hashdeep}

man_MANS=md5deep.1 sha1deep.1 sha256deep.1 tigerdeep.1 whirlpooldeep.1 hashdeep.1 #sha3deep.1

# We erase the old man pages, just in case they might be symbolic links
# (symbolic links were used in a previous release)
install-man: update-manpages
	rm -f $(DESTDIR)${man1dir}/${ALL_GOALS}deep.1
	$(MAKE) install-man1

update-manpages: md5deep.1
	$(INSTALL) $< sha1deep.1
	$(INSTALL) $< sha256deep.1
#	$(INSTALL) $< sha3deep.1
	$(INSTALL) $< tigerdeep.1
	$(INSTALL) $< whirlpooldeep.1

EXTRA_DIST=${man_MANS} 

