
CC= g++
NAME = ../scc
OBJS = scc_main.o my_work_queue.o scc_tarjan.o  scc_ufscc.o scc_color.o scc_trim1.o scc_fb_seq.o scc_post_analyze.o scc_trim2.o scc_fb_global.o scc_weak.o scc_fb_seq2.o scc_trim2_new.o scc_our_fast_trim.o scc_our_fast_trim_pra-4.o scc_our-AC4-trim.o
INCDIR = ../gm_graph/inc
LIBDIR = ../gm_graph/lib
OUTDIR = ./
LIB = gmgraph
LFLAGS = -L$(LIBDIR) -l$(LIB)
CFLAGS = -g -O3 -I. -I$(INCDIR) -fopenmp  -std=gnu++0x #-D MYDEBUG #-D DEBUG_DCSCC #-D MYDEBUG # 
INCFILES = $(shell ls $(INCDIR))
H_FILES = $(addprefix $(INCDIR)/, $(INCFILES))

all : $(NAME)

$(NAME) : $(OBJS) ../gm_graph/lib/libgmgraph.a
	$(CC) $(CFLAGS) $^ $(LFLAGS) -o $@

%.o:%.cc
	$(CC) $(CFLAGS) -c $^ -o $@

clean :
	rm scc_main.o scc scc_our_trim.o scc_our_fast_trim_pra-2.o
	
	#rm $(NAME) *.o