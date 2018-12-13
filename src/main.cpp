#include "server.hpp"

int main()
{
	nexuspool::Server server;
	if (!server.init())
	{
		return -1;
	}

	server.run();

	return 0;
}