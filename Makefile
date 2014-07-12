
all:
	make -C src
	doxygen Doxyfile

clean:
	make -C src clean
	rm -Rf docs/latex docs/html docs/man

