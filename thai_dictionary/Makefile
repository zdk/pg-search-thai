TSEARCH_DATA_DIR=`pg_config --sharedir`/tsearch_data
FILES=th_TH.dict th_TH.affix

all: copy

install: copy

copy:
	for file in $(FILES) ; do \
		cp -f hunspell/$$file $(TSEARCH_DATA_DIR);\
	done

clean:
	for file in $(FILES) ; do \
			rm -f $(TSEARCH_DATA_DIR)/$$file;\
	done
