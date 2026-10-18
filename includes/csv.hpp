#ifndef CSV_HPP
# define CSV_HPP

# include <iostream>
# include <string>
# include <fstream>
# include <vector>
# include <ctime>
# include <unordered_map>

enum cTypes
{
	STRING,
	INT,
	FLOAT,
	DATE
};

struct type
{
	cTypes		type;
	std::string	sData;
	int			iData;
	float		fData;
	struct tm	dData;
};


class columnType
{
	private:
		cTypes						_type;
		std::vector<std::string>	_sData;
		std::vector<int>			_iData;
		std::vector<float>			_fData;
		std::vector<struct tm>		_dData;

	public:
		columnType(cTypes type);
		~columnType();
	
	public:
		void	push_back(type &data);
};

columnType::columnType(cTypes type): _type(type){}

columnType::~columnType(){}

void	columnType::push_back(type &data)
{
	switch (data.type)
	{
		case STRING:
			std::cout << "String: " << data.sData << std::endl;
			_sData.push_back(data.sData);
			break;
		
		case INT:
			std::cout << "Int: " << data.iData << std::endl;
			_iData.push_back(data.iData);
			break;
		
		case FLOAT:
			std::cout << "Float: " << data.fData << std::endl;
			_fData.push_back(data.fData);
			break;
		
		case DATE:
			_dData.push_back(data.dData);
			break;
		
		default:
			throw std::runtime_error("Error: Unknown type");
			break;
	}
}

class csv
{
	private:
		char						_delim;
		std::vector<std::string>	_columnsNames;
		std::vector<columnType>		_data;

	private:
		void	fillColumnsNames(std::string &line);
		void	fillLine(std::string &line);
		void	insertData(std::string data, int numCol);
		type	analyzeType(std::string &data);

	public:
		csv(std::string pathToFile, bool columnNames=true,
			std::initializer_list<std::string> colsNames = {},
			std::initializer_list<std::string> types = {});
		~csv();
};

void	csv::fillColumnsNames(std::string &line)
{
	int comma = 0, semicol = 0;
	bool inPar = false;
	for (int i = 0; i < line.size(); i++)
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
		throw std::runtime_error("Error: double quotes must be closed");
	this->_delim = (semicol > comma) ? ';' : ',';
	int j = 0;
	int i = 0;
	for (int i = 0; i < line.size(); i++)
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

type	csv::analyzeType(std::string &data)
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
	// bool isDate = true; later
	for (auto &pair : dict)
	{
		if (isInt && !isdigit(pair.first))
			isInt = false;
		else if (isFloat && !isdigit(pair.first) && pair.first != '.')
			isFloat = false;
	}
	type res;
	if (isInt)
		res = (type){.type = INT, .iData = std::atoi(data.c_str())};
	else if (isFloat)
		res = (type){.type = FLOAT, .fData = static_cast<float>(std::atof(data.c_str()))};
	else
		res = (type){.type = STRING, .sData = data};
	return res;
}

void	csv::insertData(std::string data, int numCol)
{
	type sdata = this->analyzeType(data);
	if (static_cast<size_t>(numCol) >= this->_data.size())
		this->_data.push_back(sdata.type);
	
	this->_data[numCol].push_back(sdata);
}

void	csv::fillLine(std::string &line)
{
	bool inPar = false;
	int j = 0;
	int countCol = 0;
	int i;
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
	insertData(std::string(line, j, i - j), countCol);
}

csv::csv(std::string pathToFile, bool columnNames,
	std::initializer_list<std::string> colsNames,
	std::initializer_list<std::string> types)
{
	std::ifstream stream;
	try
	{
		stream.open(pathToFile);
		std::string line;
		int count = 0;
		while (std::getline(stream, line))
		{
			if (!count && columnNames)
				fillColumnsNames(line);
			else if (!count && !columnNames && colsNames.size())
			{
				this->_columnsNames = colsNames;
				count++;
			}
			if (count)
			{
				fillLine(line);
				return ;
			}

			count++;
		}
		stream.close();
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		stream.close();
	}
}

csv::~csv()
{
}


#endif