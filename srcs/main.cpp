#include "csv.hpp"
#include "functions.hpp"

int main(int ac, char **av)
{
	(void)ac;
	if (ac != 2)
		return 1;
	csv data = csv(av[1], false, {"begin", "phrase", "Id", "price", "end"}, {AUTO, AUTO, AUTO, DOUBLE, AUTO});
	if (!data.empty())
	{
		try
		{
			std::cout << data << std::endl;
			std::vector<float>	prices = data.getCol<float>(3);
			for (auto price : prices)
				std::cout << price << ", ";
			std::cout << std::endl;
		}
		catch(const std::exception& e)
		{
			std::cerr << e.what() << '\n';
		}
	}
	return 0;
}