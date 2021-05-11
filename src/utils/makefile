OUT_PATH := ../../build/utils

CC := g++
CC_FLAGS := -c -std=c++17

SRCS := $(wildcard *.cc)
OBJS := $(patsubst %.cc, %.o, $(SRCS))

mkdir:
	mkdir -p $(OUT_PATH)

$(OBJS): %.o: %.cc mkdir
	$(CC) $(CC_FLAGS) -o $(OUT_PATH)/$@ $<

all: $(OBJS)

clean:
	rm -r $(OUT_PATH)/*.o
