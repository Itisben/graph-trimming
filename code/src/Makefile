
CC= g++
NAME="../trim"
OBJS=scc_main.o our-AC3-trim.o our-AC4-trim.o our-AC6-trim.o
INCDIR=../gm_graph/inc
LIBDIR=../gm_graph/lib
OUTDIR=./
LIB = gmgraph
LFLAGS = -L$(LIBDIR) -l$(LIB)
CFLAGS = -O3 -I. -I$(INCDIR) -fopenmp  -std=gnu++0x #-D MYDEBUG #-D DEBUG_DCSCC #-D MYDEBUG # 
#CFLAGS = -g -I. -I$(INCDIR) -fopenmp  -std=gnu++0x #-D MYDEBUG #-D DEBUG_DCSCC #-D MYDEBUG # 
INCFILES = $(shell ls $(INCDIR))
H_FILES = $(addprefix $(INCDIR)/, $(INCFILES))

all : $(NAME)

$(NAME) : $(OBJS) ../gm_graph/lib/libgmgraph.a
	$(CC) $(CFLAGS) $^ $(LFLAGS) -o $@
	#rm *.o

%.o:%.cc
	$(CC) $(CFLAGS) -c $^ -o $@

clean :
	rm *.o