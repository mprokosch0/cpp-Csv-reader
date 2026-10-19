#include "csv.hpp"
#include "functions.hpp"

int main(int ac, char **av)
{
	(void)ac;
	if (ac != 2)
		return 1;
	auto data = csv(av[1]);
	if (!data.empty())
		std::cout << data << std::endl;
	return 0;
}