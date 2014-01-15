all:
	gcc -o test_program test_program.c -I../local/include -I../libevent-1/include -pthread ../local/lib/libzmq.a -lstdc++ ../libevent-1/lib/libevent.a
	gcc -o test_libevent2 test_libevent2.c -I../local/include -I../libevent-2/include -pthread ../local/lib/libzmq.a -lstdc++ ../libevent-2/lib/libevent.a
