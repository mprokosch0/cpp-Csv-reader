#ifndef CSV_HPP
# define CSV_HPP

# include <iostream>
# include <string>
# include <fstream>
# include <variant>
# include <vector>
# include <ctime>
# include <iomanip>
# include <sstream>
# include <cstdio>
# include <cstring>
# include <algorithm>
# include <unordered_map>
# include "bigint.hpp"

# define INT_MAX 2147483647
# define INT_MIN -2147483648

enum cTypes
{
	STRING,
	INT,
	BIGINT,
	FLOAT,
	DATE,
	AUTO,
	UNDT
};

using Cell = std::variant<
	std::monostate,   // UNDT / valeur manquante
	std::string,
	int,
	bigint,
	float,
	std::tm
>;

class columnType
{
	private:
		cTypes				_type;
		std::vector<Cell>	_data;

	private:
		void	determineCellType(Cell &cell);


	public:
		columnType(Cell &cell);
		~columnType();

	public:
		const Cell &operator[](size_t index) const;

	public:
		void	push_back(Cell &cell);
		size_t	size(void) const;
		void	reserve(size_t size);
		cTypes	getType(void) const;
};

columnType::columnType(Cell &cell)
{
	determineCellType(cell);
}

columnType::~columnType(){}

void	columnType::determineCellType(Cell &cell)
{
	if (std::get_if<std::string>(&cell))
		this->_type = STRING;
	else if (std::get_if<int>(&cell))
		this->_type = INT;
	else if (std::get_if<bigint>(&cell))
		this->_type = BIGINT;
	else if (std::get_if<float>(&cell))
		this->_type = FLOAT;
	else if (std::get_if<std::tm>(&cell))
		this->_type = DATE;
	else
		this->_type = AUTO;
}

const Cell &columnType::operator[](size_t index) const
{
	if (index >= this->_data.size())
		throw std::runtime_error("columnType: Index out of bound");
	return this->_data[index];
}

void	columnType::push_back(Cell &cell)
{
	if (this->_type == AUTO && !std::get_if<std::monostate>(&cell))
		determineCellType(cell);
	
	this->_data.push_back(cell);
}

size_t	columnType::size(void) const
{
	return this->_data.size();
}

void	columnType::reserve(size_t size)
{
	this->_data.reserve(size);
}

cTypes columnType::getType(void) const
{
	return this->_type;
}

class csv
{
	private:
		bool						_empty;
		bool						_entryTypes;
		char						_delim;
		std::vector<std::string>	_columnsNames;
		std::vector<columnType>		_data;

	private:
		void	initCsv(std::string &pathToFile, bool columnNames,
					std::initializer_list<std::string> &colsNames,
					std::initializer_list<cTypes> &types);
		void	allocate(size_t nbCols, size_t nb_rows, std::vector<cTypes> types);
		void	fillColumnsNames(std::string &line);
		void	determineDelim(std::string &line);
		void	fillLine(std::string &line);
		void	insertData(std::string data, int numCol);
		Cell	analyzeType(std::string &data);

	public:
		csv(std::string pathToFile, bool columnNames,
			std::initializer_list<std::string> colsNames = {},
			std::initializer_list<cTypes> types = {});
		csv(std::string pathToFile, bool columnNames,
			std::initializer_list<cTypes> types = {});
		csv(std::string pathToFile,
			std::initializer_list<cTypes> types);
		csv(std::string pathToFile);
		~csv();

	public:
		const std::vector<std::string> &getColsNames(void) const
		{return this->_columnsNames;}
		const std::vector<columnType> &getData(void) const
		{return this->_data;}
		bool	empty(void) const
		{return this->_empty;}
};


//print functions---------------------------------------------------

static std::string cellToString(Cell const &cell)
{
	std::ostringstream ss;
	if (auto str = std::get_if<std::string>(&cell))
		ss << *str;
	else if (auto nb = std::get_if<int>(&cell))
		ss << *nb;
	else if (auto nb = std::get_if<bigint>(&cell))
		ss << *nb;
	else if (auto f = std::get_if<float>(&cell))
		ss << *f;
	else if (auto date = std::get_if<std::tm>(&cell))
		ss << std::put_time(date, "%Y-%m-%d %H:%M:%S");
	else
		ss << "null";
	return ss.str();
}

std::ostream &operator<<(std::ostream &os, Cell const &cell)
{
	return os << cellToString(cell);
}

static void printSeparator(std::ostream &os, std::vector<size_t> const &widths)
{
	os << '+';
	for (size_t w : widths)
		os << std::string(w + 2, '-') << '+';
	os << '\n';
}

static void printRow(std::ostream &os,
					std::vector<std::string> const &cells,
					std::vector<size_t> const &widths,
					std::vector<bool> const &rightAlign)
{
	os << '|';
	for (size_t j = 0; j < widths.size(); j++)
	{
		os << ' ' << (rightAlign[j] ? std::right : std::left)
		<< std::setw(widths[j]) << cells[j] << " |";
	}
	os << '\n';
}

std::string	getColType(columnType const &col)
{
	if (col.getType() == STRING)
		return "String";
	else if (col.getType() == INT)
		return "Integer";
	else if (col.getType() == BIGINT)
		return "BigInteger";
	else if (col.getType() == FLOAT)
		return "Float";
	else if (col.getType() == DATE)
		return "DateTime";
	return "Unknown";
}

std::string	getCType(cTypes const &cType)
{
	if (cType == STRING)
		return "String";
	else if (cType == INT)
		return "Integer";
	else if (cType == BIGINT)
		return "BigInteger";
	else if (cType == FLOAT)
		return "Float";
	else if (cType == DATE)
		return "DateTime";
	return "Unknown";
}

static constexpr size_t MAX_DISPLAY_ROWS = 30;

std::ostream &operator<<(std::ostream &os, csv const &data)
{
	auto const &names = data.getColsNames();
	auto const &dataTable = data.getData();

	size_t nbCols = std::max(names.size(), dataTable.size());
	if (nbCols == 0)
		return os;

	size_t totalRows = 0;
	for (auto const &col : dataTable)
		totalRows = std::max(totalRows, col.size());

	size_t nbRows = std::min(totalRows, MAX_DISPLAY_ROWS);

	std::vector<std::string> header(nbCols);
	std::vector<std::string> types(nbCols);
	std::vector<size_t> widths(nbCols, 0);
	std::vector<bool> rightAlign(nbCols, false);

	for (size_t j = 0; j < nbCols; j++)
	{
		if (j < names.size())
			header[j] = names[j];
		types[j] = getColType(dataTable[j]);
		widths[j] = (header[j].size() > types[j].size()) ? header[j].size() : types[j].size();
	}

	std::vector<std::vector<std::string>> rows(nbRows, std::vector<std::string>(nbCols));
	for (size_t j = 0; j < dataTable.size(); j++)
	{
		for (size_t i = 0; i < std::min(dataTable[j].size(), nbRows); i++)
		{
			rows[i][j] = cellToString(dataTable[j][i]);
			widths[j] = std::max(widths[j], rows[i][j].size());
			if (std::holds_alternative<int>(dataTable[j][i]) ||
				std::holds_alternative<float>(dataTable[j][i]))
				rightAlign[j] = true;
		}
	}

	printSeparator(os, widths);
	printRow(os, header, widths, std::vector<bool>(nbCols, false));
	printRow(os, types, widths, std::vector<bool>(nbCols, false));
	printSeparator(os, widths);
	for (auto const &row : rows)
		printRow(os, row, widths, rightAlign);
	printSeparator(os, widths);

	if (totalRows > nbRows)
		os << "... " << (totalRows - nbRows) << " ligne(s) supplémentaire(s) ("
		<< totalRows << " au total)\n";

	return os;
}
#endif