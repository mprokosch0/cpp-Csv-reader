#include "csv.hpp"
#include "functions.hpp"

int main(int ac, char **av)
{
	(void)ac;
	if (ac != 2)
		return 1;
	csv data = csv(av[1], {UNDT, UNDT, UNDT, UNDT, BIGINT, UNDT});
	if (!data.empty())
		std::cout << data << std::endl;
	return 0;
}