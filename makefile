MAKE = make
CC = g++

#### normal build

all:
	cd ./src; make all
	make linking

linking:
	cd ./build; find ./ -name "*.o" | xargs g++ -o compiler

clean:
	cd ./src; make clean


#### TODO: platform build
# this requires copying all the files to ./sim/ and build them all

# copys all the ".h" and ".cc" files from ./src/* and to ./sim/
sim_copy: flex_bison make_sim
	find ./src -type f -regex ".*\.\(h\|cc\)" -exec cp {} ./sim \;
