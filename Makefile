all:
		(cd thai_dictionary; make all)
		(cd thai_parser; make; make install)

install:
		(cd thai_dictionary; make install)
		(cd thai_parser; make install)

test:
		(cd thai_parser; make test)
clean:
		(cd thai_dictionary; make clean)
		(cd thai_parser; make clean)

