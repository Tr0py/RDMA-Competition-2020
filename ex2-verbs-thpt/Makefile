CC=gcc
CFLAGS=-Wall  -g 
INCLUDES=
LDFLAGS=-libverbs
LIBS=-pthread -lrdmacm

SRCS=bw_template.c
OBJS=$(SRCS:.c=.o)
PROG=verbs-demo

all: $(PROG)

debug: CFLAGS=-Wall -g 
debug: $(PROG)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)

clean:
	$(RM) *.o *~ $(PROG)
