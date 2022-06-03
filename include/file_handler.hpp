#ifndef FILE_HANDLER_HPP
#define FILE_HANDLER_HPP

/** Beskrivning:  Header deklaration för klassen FileHandler, filehandler klassen läser in, öppnar och returnerar alla rader i en specifierat textfil.
* Argument 1:   const char* - pekare till konstant char i minnet där en sökväg till en fil ligger
* Return:       FileHandler - FileHandler objekt
* Exempel:
*               FileHandler fileHandler("known_cars.txt") => sätter privata variablen this->path till att vara "known_cars.txt" och skapar ett objekt (fileHandler)
*               fileHandler.getIDs() => ["YAH088", "MSF492"] ifall filen i sökvägen this->path innehåller två rader med var sin av följande strängar: "YAH088" och "MSF492", i kronologisk ordning
*
* By:           Vigor Turujlija Gamelius
* Date:         2022-06-03
**/
class FileHandler {
	std::string path;
public:
	FileHandler(const char *path);
	std::vector<std::string> getIDs();
 };
#endif
