CXX = gcc
TARGET := leiserchess
SRC := util.c tt.c fen.c move_gen.c move_gen_reference.c search.c eval.c abort.c speculative_add.c
OBJ := $(addsuffix .o, $(basename $(SRC)))

ifeq ($(PARALLEL),1)
	PFLAG := -DPARALLEL # -g needed for test framework assertions
else
	PFLAG := 
endif

ifeq ($(TEST),1)
	CFLAGS := -Wall -O0 -g -DTEST $(PFLAG) -fcilkplus # -g needed for test framework assertions
else
	CFLAGS := -Wall -O3 -DNDEBUG $(PFLAG) -fcilkplus
endif

LDFLAGS= -Wall -lrt -lm -lcilkrts -ldl

%.o : %.c
	$(CXX) -std=c99 -c $(CFLAGS) $(LDFLAGS) $< -o $@

leiserchess : $(OBJ) leiserchess.o
	$(CXX) $(OBJ) leiserchess.o $(LDFLAGS) -o $@


clean :
	rm -f *.o *~ $(TARGET)
