#ifndef FUNCTIONS_HPP

# define FUNCTIONS_HPP

#include "csv.hpp"


void	csv::determineDelim(std::string &line)
{
	int comma = 0, semicol = 0;
	bool inQuote = false;
	for (size_t i = 0; i < line.size(); i++)
	{
		char c = line[i];
		if (c == '\"')
		{
			if (inQuote && i + 1 < line.size() && line[i + 1] == '\"')
				i++;
			else
				inQuote = !inQuote;
		}
		else if (c == ';')
			semicol++;
		else if (c == ',')
			comma++;
	}
	if (inQuote)
		throw std::runtime_error("in cell, double quotes must be closed");
	this->_delim = (semicol > comma) ? ';' : ',';
}

std::vector<std::string>	splitLines(std::string const &line, char delim)
{
	std::vector<std::string> res;
	std::string word;
	bool inQuote = false;
	for (size_t i = 0; i < line.size(); i++)
	{
		char c = line[i];
		if (c == '\"')
		{
			if (inQuote && i + 1 < line.size() && line[i + 1] == '\"')
			{
				word += '\"';
				i++;
			}
			else
				inQuote = !inQuote;
		}
		else if (c == delim && !inQuote)
		{
			res.push_back(word);
			word.clear();
		}
		else
			word += c;
	}
	if (inQuote)
		throw std::runtime_error("in cell, double quotes must be closed");
	res.push_back(word);
	return res;
}

void	csv::fillColumnsNames(std::string &line)
{
	determineDelim(line);
	this->_columnsNames = splitLines(line, this->_delim);
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

Cell csv::analyzeType(std::string &s)
{
	if (s.empty())
		return std::monostate();

	const char *b = s.c_str();
	char *end = nullptr;

	errno = 0;
	long long v = std::strtoll(b, &end, 10);
	if (*end == '\0')
	{
		if (errno == ERANGE || v > INT_MAX || v < INT_MIN)
			return bigint(s);
		return static_cast<int>(v);
	}

	errno = 0;
	double d = std::strtod(b, &end);
	if (*end == '\0' && errno != ERANGE && std::isfinite(d))
		return static_cast<float>(d);

	Cell date;
	if (parse_tm(s, date))
		return date;

	return s;
}

cTypes	resolveType(Cell const &cell)
{
	if (std::get_if<std::string>(&cell))
		return STRING;
	else if (std::get_if<int>(&cell))
		return INT;
	else if (std::get_if<bigint>(&cell))
		return BIGINT;
	else if (std::get_if<float>(&cell))
		return FLOAT;
	else if (std::get_if<std::tm>(&cell))
		return DATE;
	return UNDT;
}

void	csv::insertData(std::string data, int numCol)
{
	cTypes colType = this->_data[numCol].getType();
	Cell sdata = data;
	if (colType != STRING)
		sdata = this->analyzeType(data);
	else if (!data.size())
		sdata = std::monostate();
	if (/*this->_entryTypes && */colType != AUTO)
	{
		cTypes cellType = resolveType(sdata);
		if (colType == BIGINT && cellType == INT)
		{
			sdata = bigint(*std::get_if<int>(&sdata));
			cellType = BIGINT;
		}
		if (colType == FLOAT && cellType == INT)
		{
			sdata = static_cast<float>(std::get<int>(sdata));
			cellType = FLOAT;
		}
		if (colType != cellType && cellType != UNDT)
			throw std::runtime_error("Data in line " + std::to_string(this->_data[numCol].size()) + " column " + std::to_string(numCol)
					+ " isn't from the specified type: got: " + getCType(cellType) + ", exepected: " + getCType(colType));
	}
	
	this->_data[numCol].push_back(sdata);
}

void	csv::fillLine(std::string &line)
{
	auto words = splitLines(line, this->_delim);
	if (this->_columnsNames.size() < words.size())
		throw std::runtime_error("Too few cols names for the data");
	if (this->_columnsNames.size() > words.size())
		throw std::runtime_error("Too few data in line " + std::to_string(this->_data[0].size()));
	size_t colnum = 0;
	for (std::string &word : words)
		insertData(word, colnum++);
}

size_t countLines(const std::string &path)
{
	std::ifstream f(path, std::ios::binary);
	return std::count(std::istreambuf_iterator<char>(f),
					std::istreambuf_iterator<char>(), '\n');
}

void	csv::allocate(size_t nbCols, size_t nbRows, std::vector<cTypes> types)
{
	if (types.size() && this->_columnsNames.size() > types.size())
		throw std::runtime_error("Too few types given in input");

	bool isTypes = false;

	if (types.size())
		isTypes = true;
	
	this->_data.reserve(nbCols);
	Cell c = std::monostate();
	for (size_t i = 0; i < nbCols; i++)
	{
		if (isTypes)
		{
			switch (types[i])
			{
				case STRING:
					c = std::string();
					break;
				
				case INT:
					c = int();
					break;
				
				case BIGINT:
					c = bigint();
					break;
				
				case FLOAT:
					c = float();
					break;
				
				case DATE:
					c = std::tm();
					break;

				case AUTO:
					c = std::monostate();
					break;
			
				default:
					throw std::runtime_error("UNDT cannot be a col type");
					break;
			}
		}
		this->_data.emplace_back(c);
		this->_data.back().reserve(nbRows);
	}
}

void	csv::initCsv(std::string &pathToFile, bool columnNames,
	std::initializer_list<std::string> &colsNames,
	std::initializer_list<cTypes> &types)
{
	std::ifstream stream;
	try
	{
		if (!columnNames && !colsNames.size())
			throw std::runtime_error("Argument colsNames should not be empty");
		size_t nbRows = countLines(pathToFile);
		std::string line;

		stream.open(pathToFile);
		if (!stream.is_open())
			throw std::runtime_error("Couldn't open the file");
		if (columnNames)
		{
			std::getline(stream, line);
			fillColumnsNames(line);
		}
		else if (!columnNames && colsNames.size())
			this->_columnsNames = colsNames;
		allocate(this->_columnsNames.size(), nbRows, types);
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

csv::csv(std::string pathToFile, bool columnNames,
	std::initializer_list<std::string> colsNames,
	std::initializer_list<cTypes> types): _empty(false), _entryTypes(types.size() > 0)
{
	this->_delim = 0;
	initCsv(pathToFile, columnNames, colsNames, types);
}

csv::csv(std::string pathToFile, bool columnNames,
	std::initializer_list<cTypes> types): _empty(false), _entryTypes(types.size() > 0)
{
	this->_delim = 0;
	std::initializer_list<std::string> tmp({});
	initCsv(pathToFile, columnNames, tmp, types);
}

csv::csv(std::string pathToFile,
	std::initializer_list<cTypes> types): _empty(false), _entryTypes(types.size() > 0)
{
	this->_delim = 0;
	std::initializer_list<std::string> tmp({});
	initCsv(pathToFile, true, tmp, types);
}

csv::csv(std::string pathToFile): _empty(false), _entryTypes(false)
{
	this->_delim = 0;
	std::initializer_list<std::string> tmp({});
	std::initializer_list<cTypes> tmp2({});
	initCsv(pathToFile, true, tmp, tmp2);
}

csv::~csv(){}

#endif