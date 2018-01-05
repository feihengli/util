
include ../Rules.make

LIB_NAME=libipc

all:
	@rm -rf $(LIB_NAME).*
	@for i in `ls *.a`; do\
	   { $(AR) -x $$i;}& done; wait
	$(CC) -shared -fPIC -o $(LIB_NAME).so *.o
	$(AR) rcs -o $(LIB_NAME).a *.o
	$(STRIP) $(LIB_NAME).*
	@rm -rf *.o
clean:
	@rm -rf $(LIB_NAME).*

