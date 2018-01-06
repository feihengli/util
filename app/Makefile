

all:
	@for x in `find ./ -maxdepth 2 -mindepth 2 -name "Makefile" `; do\
	   { cd `dirname $$x`; if [ $$? ]; then make || exit 1; cd ../; fi;}& done; wait

clean:
	@for x in `find ./ -maxdepth 2 -mindepth 2 -name "Makefile" `; do\
	   { cd `dirname $$x`; if [ $$? ]; then make clean; cd ../; fi;}& done; wait