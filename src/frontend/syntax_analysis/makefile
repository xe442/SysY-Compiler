OUT_PATH := ../../../build/frontend/syntax_analysis
UTILS_PATH := ../../utils

CC := g++
CC_FLAGS := -c -lfl -std=c++17 -I $(UTILS_PATH)

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

lexer_parser: lexer.yy.cc parser.tab.cc

lexer_parser_clean:
	rm -f lexer.yy.cc parser.tab.h parser.tab.cc $(UTILS_PATH)/location.h

$(OBJS): %.o: %.cc
	$(CC) $(CC_FLAGS) -o $(OUT_PATH)/$@ $<

all: lexer.yy.o parser.tab.o $(OBJS)

# graphics: parser.y
#	$(BISON) $(BISON_FLAGS) -g -o $@ $^
#	dot -Tpng -o parser.png parser.dot
