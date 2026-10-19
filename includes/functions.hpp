#ifndef FUNCTIONS_HPP

# define FUNCTIONS_HPP

#include "csv.hpp"


void	csv::determineDelim(std::string &line)
{
	int comma = 0, semicol = 0;
	bool inPar = false;
	for (size_t i = 0; i < line.size(); i++)
	{
		if (line[i] == '\"' && !inPar)
			inPar = true;
		else if (line[i] == '\"' && inPar
			&& ((i < line.size() - 1 && line[i + 1] != '\"') || (i == line.size() - 1)))
			inPar = false;
		if (inPar)
			continue ;
		if (line[i] == ';')
			semicol++;
		if (line[i] == ',')
			comma++;
	}
	if (inPar)
		throw std::runtime_error("in cell, double quotes must be closed");
	this->_delim = (semicol > comma) ? ';' : ',';
}

void	csv::fillColumnsNames(std::string &line)
{
	determineDelim(line);
	bool inPar = false;
	int j = 0;
	size_t i = 0;
	for (i = 0; i < line.size(); i++)
	{
		if (line[i] == '\"' && !inPar)
			inPar = true;
		else if (line[i] == '\"' && inPar
			&& ((i < line.size() - 1 && line[i + 1] != '\"') || (i == line.size() - 1)))
			inPar = false;
		if (inPar)
			continue ;
		if (line[i] == this->_delim)
		{
			this->_columnsNames.push_back(std::string(line, j, i - j));
			j = i + 1;
		}
	}
	this->_columnsNames.push_back(std::string(line, j, i - j));
}

bool parse_tm(const std::string& s, Cell& out)
{
	int Y, M, D, h, m, sec, n = 0;
	char tz[8] = {};

	if (std::sscanf(s.c_str(), "%4d-%2d-%2d %2d:%2d:%2d %7s%n",
					&Y, &M, &D, &h, &m, &sec, tz, &n) != 7)
		return false;
	if (n != (int)s.size() || std::strcmp(tz, "UTC") != 0)
		return false;
	if (M < 1 || M > 12 || D < 1 || D > 31 || h > 23 || m > 59 || sec > 60)
		return false;

	std::tm tm{};
	tm.tm_year = Y - 1900;
	tm.tm_mon  = M - 1;
	tm.tm_mday = D;
	tm.tm_hour = h;
	tm.tm_min  = m;
	tm.tm_sec  = sec;

	std::tm check = tm;
	#ifdef _WIN32
		if (_mkgmtime(&check) == -1)
			return false;
	#else
		if (timegm(&check) == -1)
			return false;
	#endif
		if (check.tm_mday != D || check.tm_mon != M - 1)
			return false;

	out = check;
	return true;
}

Cell	csv::analyzeType(std::string &data)
{
	std::unordered_map<char, int> dict;
	for (char c : data)
	{
		if (dict.find(c) != dict.end())
			dict.at(c) += 1;
		else
			dict.insert(std::make_pair(c, 0));
	}
	bool isInt = true;
	bool isFloat = true;
	bool isDate = true;
	for (auto &pair : dict)
	{
		if (isInt && !isdigit(pair.first))
			isInt = false;
		else if (isFloat && !isdigit(pair.first) && pair.first != '.')
			isFloat = false;
		else if (isDate && !isdigit(pair.first) && pair.first != '-' && pair.first != ' '
					&& pair.first != ':' && pair.first != 'U' && pair.first != 'T' && pair.first != 'C'
					&& pair.first != '+' && pair.first != '-')
			isDate = false;
	}
	Cell res;
	if (!data.size())
		Cell res = std::monostate();
	else if (isInt)
	{
		float r = std::atof(data.c_str());
		if (r >= __INT_MAX__ || r <= -__INT_MAX__ - 1)
			res = bigint(data);
		else
			res = std::atoi(data.c_str());
	}
	else if (isFloat)
		res = static_cast<float>(std::atof(data.c_str()));
	else if (isDate && parse_tm(data, res))
		;
	else
		res = data;
	return res;
}

void	csv::insertData(std::string data, int numCol)
{
	Cell sdata = this->analyzeType(data);
	if (static_cast<size_t>(numCol) >= this->_data.size())
		this->_data.push_back(columnType(sdata));
	
	this->_data[numCol].push_back(sdata);
}

void	csv::fillLine(std::string &line)
{
	bool inPar = false;
	int j = 0;
	size_t countCol = 0;
	size_t i;
	for (i = 0; i < line.size() && countCol < this->_columnsNames.size(); i++)
	{
		if (line[i] == '\"' && !inPar)
			inPar = true;
		else if (line[i] == '\"' && inPar
			&& ((i < line.size() - 1 && line[i + 1] != '\"') || (i == line.size() - 1)))
			inPar = false;
		if (inPar)
			continue ;
		if (line[i] == this->_delim)
		{
			insertData(std::string(line, j, i - j), countCol);
			j = i + 1;
			countCol++;
		}
	}
	if (countCol > this->_columnsNames.size() - 1)
		throw std::runtime_error("Too few cols names for the data");
	insertData(std::string(line, j, i - j), countCol);
	if (countCol + 1 < this->_columnsNames.size())
		throw std::runtime_error("Too few data in the line");
}

size_t countLines(const std::string &path)
{
	std::ifstream f(path, std::ios::binary);
	return std::count(std::istreambuf_iterator<char>(f),
					std::istreambuf_iterator<char>(), '\n');
}

void	csv::allocate(size_t nbCols, size_t nbRows)
{
	this->_data.reserve(nbCols);
	Cell c = std::monostate();
	for (size_t i = 0; i < nbCols; i++)
	{
		this->_data.emplace_back(c);
		this->_data.back().reserve(nbRows);
	}
}

csv::csv(std::string pathToFile, bool columnNames,
	std::initializer_list<std::string> colsNames,
	std::initializer_list<std::string> types): _empty(false)
{
	std::ifstream stream;
	this->_delim = 0;
	(void)types;
	try
	{
		size_t nbRows = countLines(pathToFile);
		std::string line;

		stream.open(pathToFile);
		if (columnNames)
		{
			std::getline(stream, line);
			fillColumnsNames(line);
		}
		else if (!columnNames && colsNames.size())
			this->_columnsNames = colsNames;
		allocate(this->_columnsNames.size(), nbRows);
		while (std::getline(stream, line))
		{
			if (this->_delim == 0)
				determineDelim(line);
			fillLine(line);
		}
		stream.close();
	}
	catch(const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << '\n';
		this->_columnsNames.clear();
		this->_data.clear();
		this->_delim = 0;
		this->_empty = true;
		stream.close();
	}
}

csv::~csv(){}

#endif