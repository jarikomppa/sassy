/*
* sassy-asio wrapper, to make it possible to tear asio out easily.
*/

#include "sassy.h"

int asio_devicecount()
{
	return 0;
}

const char* asio_devicename(int)
{
	return 0;
}


void asio_init(char*, int)
{
}

void asio_deinit()
{
}