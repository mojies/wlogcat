all:
	rm -rf test
	g++ --std=c++17 -o test main.cpp params.c tools.c exe.c logcat.c str.c
	# ./test -h
