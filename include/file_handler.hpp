#ifndef FILE_HANDLER_HPP
#define FILE_HANDLER_HPP

class FileHandler {
	std::string path;
public:
	FileHandler(const char *path);
	std::vector<std::string> getIDs();
 };
#endif
