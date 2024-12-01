//=============================================================================//
//               DELTA2SLIKS - Converts from DELTA to SLIKS format             //
//                    Copyright 2024 Mauro J. Cavalcanti                       //
//                          maurobio@gmail.com                                 //
//                                                                             //
//      This program is free software: you can redistribute it and/or modify   //
//      it under the terms of the GNU General Public License as published by   //
//      the Free Software Foundation, either version 3 of the License, or      //
//      (at your option) any later version.                                    //
//                                                                             //
//      This program is distributed in the hope that it will be useful,        //
//      but WITHOUT ANY WARRANTY; without even the implied warranty of         //
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          //
//      GNU General Public License for more details.                           //
//                                                                             //
//      You should have received a copy of the GNU General Public License      //
//      along with this program. If not, see <http://www.gnu.org/licenses/>.   //
//                                                                             //
//   Requirements:                                                             //
//      GNU g++ compiler v3.x or higher                                        //
//      tDelta Class Library v0.20.2 by Denis Ziegler                          // 
//      with amendments by Guillaume Rousse and Bastiaan Wakkie                // 
//      available from                                                         //  
//      https://sourceforge.net/projects/freedelta/files/deltalib/new/         //
//                                                                             //
//  REVISION HISTORY:                                                          //
//      Version 1.0, 1st Dec 2024 - Initial version                            //
//=============================================================================//

#include <string>
#include <iostream>
#include <cctype>
#include <algorithm>
#include "tdelta.h"

using namespace std;

// Function to remove trailing chars from an string
std::string trim(const std::string &s) {
    auto start = s.begin();
    while (start != s.end() && std::isspace(*start)) {
        start++;
    }
    auto end = s.end();
    do {
        end--;
    } while (std::distance(start, end) > 0 && std::isspace(*end));
    return std::string(start, end + 1);
}

// Function to parse a DELTA attribute in the format c,v
// where c is a character number and v is a character value
std::string parse_attribute(const std::string& input) {
    size_t commaPos = input.find(','); // Find the position of the comma
    if (commaPos != std::string::npos) {
        std::string attributeValue = input.substr(commaPos + 1); // Get the part after the comma
        // Check if attributeValue contains only digits
        if (std::all_of(attributeValue.begin(), attributeValue.end(), ::isdigit)) {
            return attributeValue;
        }
    }
    return "?"; // Return an interrogation mark if no comma or invalid format
}

// Run this program using the console pauser or add your own getch, system("pause") or input loop 
int main(int argc, char** argv) {
	tDelta *Dataset;
  	
  	// Verify filenames arguments
  	if (argc < 3) {
    	cout << "Usage : sliks <char_filename> <item_filename> [<specs_filename>]" << endl;
    	return 0;
  	}
  	
  	// Creates CharList, ItemList and Specs objects and parses the corresponding text files 
  	if (argc == 3)
    	Dataset = new tDelta(argv[1], argv[2]);   // no specifications file
  	else
    	Dataset = new tDelta(argv[1], argv[2], argv[3]);
    	
    if (!(Dataset->chars->is_parsed() && Dataset->items->is_parsed())) {
    	cout << "Error parsing characters and/or items description files" << endl;
    	delete Dataset;
    	return 1;
  	}
  
  	if (Dataset->specs && !Dataset->specs->is_parsed()) {
    	cout << "Error parsing specifications file" << endl;
    	delete Dataset;
    	return 1;
  	}
  	
  	cout << "==================" << endl;
  	cout << "Free Delta Project" << endl;
  	cout << "==================" << endl;
	cout << "Convert from DELTA to SLIKS format" << endl << endl;
  	
  	cout << "File \"" << Dataset->chars->get_filename() << "\" parsed" << endl;
	cout << "*** " << Dataset->chars->get_chars_nb() << " characters ***" << endl << endl;
  	cout << "File \"" << Dataset->items->get_filename() << "\" parsed" << endl;
	cout << "*** " << Dataset->items->get_items_nb() << " items ***" << endl << endl;
  	if (Dataset->specs)
    	cout << "File \"" << Dataset->specs->get_filename() << "\" parsed" << endl;
    cout << endl;
    
    // Get title from characters file
	ifstream infile(Dataset->chars->get_filename()); // Replace with your file's name
    if (!infile.is_open()) {
        cout << "Error: Could not open the characters file!" << endl;
        delete Dataset;
        return 1;
    }

    string line;
    string command = "*SHOW";
    string title = ""; // Declare outside the loop for broader scope
    
	while (getline(infile, line)) {
        // Check if the line starts with *SHOW
        if (line.rfind(command, 0) == 0) {
            // Extract and trim the part after the command
            title = line.substr(command.length()); // Extract after *COMMENT
            title = title.empty() ? title : title.substr(1); // Remove leading space
            cout << "Extracted title: \"" << title << "\"" << endl;
            break;
        }
    }
    infile.close();
    
    // Get numeric and text characters from CharList
	string excluded = "";
    for (int i = 1; i <= Dataset->chars->get_chars_nb(); i++) {  
    	if (Dataset->chars->get_char_type(i) == CT_IN || Dataset->chars->get_char_type(i) == CT_RN || Dataset->chars->get_char_type(i) == CT_TE)
    		excluded += to_string(i) + " ";
	}

	// Close the original dataset
	delete Dataset;
    
    // Generate CONFOR directives file to exclude numeric and text characters
    ofstream dirfile;
    dirfile.open("delchars", ios::out); // Just yet another way of opening a file
    dirfile << "*SHOW ~ Translate into DELTA format, omitting numeric and text characters\n" << endl;
    dirfile << "*LISTING FILE delchars.lst\n" << endl;
    dirfile << "*INPUT FILE specs_viola\n" << endl;
    dirfile << "*EXCLUDE CHARACTERS " << excluded << endl << endl;
    dirfile << "*TRANSLATE INTO DELTA FORMAT\n" << endl;
    dirfile << "*OUTPUT FILE chars.new" << endl;
    dirfile << "*OUTPUT PARAMETERS" << endl;
    dirfile << "#SHOW " << title << endl << endl;
    dirfile << "#CHARACTER LIST" << endl;
    dirfile << "*INPUT FILE chars_viola\n" << endl;
    dirfile << "*OUTPUT FILE items.new" << endl;
    dirfile << "*OUTPUT PARAMETERS\n" << endl;
    dirfile << "#ITEM DESCRIPTIONS" << endl;
    dirfile << "*INPUT FILE items_viola";
    dirfile.close();
	
    // Run CONFOR to exclude numeric and text characters
    #if defined(_WIN32) 
	int result = system("confor delchars");
	#elif defined(__linux__)	
    int result = system("./confor delchars");
    #endif
    if (result != 0) {
        cout << "Error: CONFOR execution failed!" << endl;
        return 1;
    }
    
    // Open the new trimmed DELTA dataset
    Dataset = new tDelta("chars.new", "items.new");
    	
    if (!(Dataset->chars->is_parsed() && Dataset->items->is_parsed())) {
    	cout << "Error parsing characters and/or items description files" << endl;
    	delete Dataset;
    	return 1;
  	}
  
    // Translate into SLIKS format
    ofstream outfile("data.js");
    outfile << "var dataset = \"<h2>" << title << "</h2>" << "\"" << endl << endl;
    
    // Output characters list
  	outfile << "var chars = [ [ \"Latin Name\"],"  << endl;
  	for (int i = 1; i <= Dataset->chars->get_chars_nb(); i++) {  
        	outfile << "\t[ \"" << Dataset->chars->get_char_feature(i) << "\", ";
			for (int j = 1; j <= Dataset->chars->get_states_nb(i); j++) {  
				if (j < Dataset->chars->get_states_nb(i)) 
					outfile << "\"" << Dataset->chars->get_state(i, j) << "\", ";
				else
					outfile << "\"" << Dataset->chars->get_state(i, j) << "\"";
			} 
			if (i < Dataset->chars->get_chars_nb())
				outfile << "],";
			else
				outfile << "] ]";	
			outfile << endl;	
	}

	// Output data matrix
	outfile << "\n\nvar items = [ [\"\"],\n";
	for (int i = 1; i <= Dataset->items->get_items_nb(); i++) {
        outfile << "\t[\"" << trim(Dataset->items->get_item_name(i, 0)) << "\", ";
        for (int j = 1; j <= Dataset->items->get_attributes_nb(i); j++) {        		
        	if (j < Dataset->items->get_attributes_nb(i))
            	outfile << "\"" << parse_attribute(Dataset->items->get_attribute(i, j)) << "\",";
            else
				outfile << "\"" << parse_attribute(Dataset->items->get_attribute(i, j)) << "\"";
    	}
        if (i < Dataset->items->get_items_nb())    
        	outfile << "],";
        else
			outfile << "]";
		outfile << endl;	
	}

	outfile.close();  	
  	delete Dataset;	
	return 0;
}
