USR_PROG     = sf.exe
USR_SRCS     = main.c utils.c view.c 
#USR_LIBS     = -lsqlite3 -Wl,-Bstatic -lcfu
USR_LIBS     = -lcfu -lmagic
USR_INCLUDES = -I $(HOME)/clang/include/cfu

USR_OBJS = $(USR_SRCS:.c=.o)
CFLAGS   =
LDFLAGS  = -L $(HOME)/clang/lib/

run:	$(USR_PROG)
	sleep 2
	#./$(USR_PROG)  /user/bluestone/flutter
	#./$(USR_PROG)  /user/bluestone
	#./$(USR_PROG)  /user/rseward/syncthing/bluestone/
	#./$(USR_PROG)  /user/rseward/src/play/
	#./$(USR_PROG)  /home

$(USR_PROG):	$(USR_OBJS) $(USR_SRCS)
	gcc $(LDFLAGS)  -o $(USR_PROG) $(USR_OBJS) $(USR_LIBS) 
	ls -l $(USR_PROG)

.c.o:
	gcc $(CFLAGS) $(USR_INCLUDES) -c $<

clean:
	rm -f $(USR_OBJS) $(USR_PROG)

format:
	astyle --style=gnu --suffix=none --verbose *.c *.h

