all: lib lib/libmmalloc.so demo

lib:
	[ -d lib ] || mkdir -p lib

mmalloc:
	[ -d mmalloc ] || mkdir -p mmalloc
	@cp Makefile.mmalloc mmalloc/Makefile
	@cp dlmalloc/malloc.c mmalloc/
	@cp dlmalloc/malloc.h mmalloc/

lib/libmmalloc.so: mmalloc/libmmalloc.so
	cp mmalloc/libmmalloc.so lib/libmmalloc.so

mmalloc/libmmalloc.so: mmalloc
	@patch -p0 < mmalloc.c.patch
	(cd mmalloc && make libmmalloc.so)

demo: lib
	(cd msServerDemo && make)
	(cd msClientDemo && make)

tags:
	(cd msServerDemo && make tags)
	(cd msClientDemo && make tags)

clean:
	(cd msServerDemo && make clean)
	(cd msClientDemo && make clean)
	rm -rf mmalloc
	rm -rf lib

cleanall: clean
	(cd msServerDemo && make clean-tags)
	(cd msClientDemo && make clean-tags)
