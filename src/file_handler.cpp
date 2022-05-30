#include <fstream>
#include <string>
#include <vector>
#include "file_handler.hpp"

FileHandler::FileHandler(const char *path_in) {
	this->path = path_in;
}

std::vector<std::string> FileHandler::getIDs(){
	std::vector<std::string> ids;
	std::ifstream file(this->path.c_str());
	std::string str;
	while (std::getline(file, str)) {
		ids.push_back(str);
	}
	return ids;
}
