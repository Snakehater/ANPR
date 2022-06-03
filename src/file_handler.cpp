#include <fstream>
#include <string>
#include <vector>
#include "file_handler.hpp"

/** Beskrivning:  Detta är en konstruktor som sätter privata variabler i klassen FileHandler
* Argument 1:   const char* - pekare till konstant char i minnet där en sökväg till en fil ligger
* Return:       FileHandler - FileHandler objekt
* Exempel:
*               FileHandler fileHandler("known_cars.txt") => sätter privata variablen this->path till att vara "known_cars.txt" och skapar ett objekt (fileHandler)
*
* By:           Vigor Turujlija Gamelius
* Date:         2022-06-03
**/
FileHandler::FileHandler(const char *path_in) {
	this->path = path_in;
}

/** Beskrivning:  Funktionen använder den privata variablen this->path och öppnar på så vis filen där varje rad delas upp i element av en vector som sedan retuneras
* Return:       std::vector<std::string> - vector av strängar där varje sträng är en rad i textfilen på sökvägen this->path
* Exempel:
*               getIDs() => ["YAH088", "MSF492"] ifall filen i sökvägen this->path innehåller två rader med var sin av följande strängar: "YAH088" och "MSF492", i kronologisk ordning
*
* By:           Vigor Turujlija Gamelius
* Date:         2022-06-03
**/
std::vector<std::string> FileHandler::getIDs(){
	std::vector<std::string> ids;
	std::ifstream file(this->path.c_str());
	std::string str;
	while (std::getline(file, str)) {
		ids.push_back(str);
	}
	return ids;
}
