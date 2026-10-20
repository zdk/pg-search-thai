TSEARCH_DATA_DIR=`pg_config --sharedir`/tsearch_data
FILES=th_TH.dict th_TH.affix

all: copy

install: copy

# Installed under lowercase names (th_th.dict, th_th.affix). PostgreSQL only
# opens text search files whose names are lowercase a-z, 0-9 and _, and it
# folds DictFile = th_TH to th_th, so on a case-sensitive file system a file
# named th_TH.dict can never be found.
copy:
	for file in $(FILES) ; do \
		cp -f hunspell/$$file $(TSEARCH_DATA_DIR)/`echo $$file | tr A-Z a-z`;\
	done

clean:
	for file in $(FILES) ; do \
			rm -f $(TSEARCH_DATA_DIR)/$$file $(TSEARCH_DATA_DIR)/`echo $$file | tr A-Z a-z`;\
	done
