# File              : Makefile
# Author            : Igor V. Sementsov <ig.kuzm@gmail.com>
# Date              : 06.12.2021
# Last Modified Date: 02.04.2022
# Last Modified By  : Igor V. Sementsov <ig.kuzm@gmail.com>


all:
	mkdir -p build && cd build && cmake -DycPhoto_BUILD_TEST="1" .. && make && open ycPhoto_test


clean:
	rm -fr build
