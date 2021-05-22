OUT_PATH := ../build
UTILS_PATH := ./utils
DEFINE_PATH := ./define
FRONTEND_PATH := ./frontend
BACKEND_EEYORE_PATH := ./backend/eeyore

CC := g++
CC_FLAGS := -c -lfl -std=c++17 -I $(UTILS_PATH) -I $(DEFINE_PATH) -I $(FRONTEND_PATH) -I $(BACKEND_EEYORE_PATH)

main.o: main.cc
	$(CC) $(CC_FLAGS) -I ./ -o $(OUT_PATH)/$@ $<

all:
	cd ./utils; make all
	cd ./define; make all
	cd ./frontend; make all
	cd ./backend/eeyore; make all
	make main.o

clean:
	rm -rf ../build
