OUT_PATH := ../../build/frontend
UTILS_PATH := ../utils
DEFINE_PATH := ../define

CC := g++
CC_FLAGS := -c -lfl -std=c++17 -I $(UTILS_PATH) -I $(DEFINE_PATH)

FLEX := flex
FLEX_FLAGS :=

BISON := bison
BISON_FLAGS :=

SRCS := $(wildcard *.cc)
OBJS := $(patsubst %.cc, %.o, $(SRCS))

# pre-defined variables:
# $@  --  the output file name
# $<  --  the first input file name
# $^  --  all the input file names

mkdir:
	mkdir -p $(OUT_PATH)

lexer.yy.cc: lexer.l mkdir
	$(FLEX) $(FLEX_FLAGS) -o $@ $<

# this also generates parser.tab.h and location.h
parser.tab.cc: parser.y mkdir
	$(BISON) $(BISON_FLAGS) $<

lexer.yy.o: lexer.yy.cc mkdir parser.tab.cc
	$(CC) $(CC_FLAGS) -o $(OUT_PATH)/$@ $<

parser.tab.o: parser.tab.cc mkdir
	$(CC) $(CC_FLAGS) -o $(OUT_PATH)/$@ $<

$(OBJS): %.o: %.cc lexer.yy.cc parser.tab.cc
	$(CC) $(CC_FLAGS) -o $(OUT_PATH)/$@ $<

all: lexer.yy.o parser.tab.o $(OBJS)

clean:
	rm -f lexer.yy.cc parser.tab.cc parser.tab.h $(OUT_PATH)/*

# utils:

# graphics: parser.y
#	$(BISON) $(BISON_FLAGS) -g -o $@ $^
#	dot -Tpng -o parser.png parser.dot
