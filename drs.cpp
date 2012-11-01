/*
 *  This file is part of ExtractDRS.
 *
 *  ExtractDRS is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  ExtractDRS is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with ExtractDRS.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file drs.cpp
 * This file deals with extracting .drs files
 */

#include "drs.h"

using namespace std;

/**
 * Converts (a part of) a string to a 4 byte uint
 * @param str The string to operate on
 * @param offset How far into the string to start
 * @return The converted uint
 */
uint str2uint(const string &str, int offset)
{
	return (unsigned char)str[offset] + ((unsigned char)str[offset + 1] << 8) + ((unsigned char)str[offset + 2] << 16) + ((unsigned char)str[offset + 3] << 24);
}

/**
 * Create a directory with the given name
 * @note Taken from the OpenTTD project
 * @param name The name of the new directory
 */
void FioCreateDirectory(const char *name)
{
#if defined(WIN32) || defined(WINCE)
	CreateDirectory(name, NULL);
#elif defined(OS2) && !defined(__INNOTEK_LIBC__)
	mkdir(name);
#elif defined(__MORPHOS__) || defined(__AMIGAOS__)
	size_t len = strlen(name) - 1;
	if (name[len] '/') {
		name[len] = '\0'; // Don't want a path-separator on the end
	}

	mkdir(name, 0755);
#else
	mkdir(name, 0755);
#endif
}

/**
 * Actually extract the drs file
 * @param path The path to the drs file
 */
void ExtractDRSFile(string path)
{
	string filename = path.substr(path.find(PATHSEP) + 1, path.length());
	ifstream file;
	cout << "Reading " << path << ":\n";
	file.open(path.c_str(), ios::in | ios::binary | ios::ate);
	if (!file.is_open()) {
		cout << "Error opening file: " << path << '\n';
		return;
	}

	/* Get the whole file */
	int size = (int)file.tellg();

	if (size < HEADER_SIZE) {
		cout << "File is too small: Only " << size << " bytes long\n";
	}
	unsigned char *memblock = new unsigned char[size];
	file.seekg(0);
	file.read((char *)memblock, size);
	file.close();
	string drstext(reinterpret_cast<char *>(memblock), size);
	delete[] memblock;

	/* Get the header */
	Header header;
	string headertext = drstext.substr(0, HEADER_SIZE);
	header.copyright = headertext.substr(0, COPYRIGHT_SIZE);
	header.version = headertext.substr(COPYRIGHT_SIZE, VERSION_SIZE);
	header.type = headertext.substr(COPYRIGHT_SIZE + VERSION_SIZE, TYPE_SIZE);
	header.numtables = str2uint(headertext, 56);
	header.firstoffset = str2uint(headertext, 60);

	/* Get tables */
	TableInfo *tableinfos = new TableInfo[header.numtables];
	for (uint i = 0; i < header.numtables; i++) {
		string tableinfotext = drstext.substr(HEADER_SIZE + (i * TABLE_SIZE), TABLE_SIZE);
		tableinfos[i].character = tableinfotext[0];

		/* Reorder the extension */
		tableinfos[i].extension = tableinfotext.substr(1, 3);
		char tmp = tableinfos[i].extension[0];
		tableinfos[i].extension[0] = tableinfos[i].extension[2];
		tableinfos[i].extension[2] = tmp;

		tableinfos[i].tbloffset = str2uint(tableinfotext, 4);
		tableinfos[i].numfiles = str2uint(tableinfotext, 8);

		cout << "TableInfo No." << i + 1 << ":\n";
		cout << "\tExtension: " << tableinfos[i].extension << '\n';
		cout << "\tNumber of files: " << tableinfos[i].numfiles << '\n';

		tableinfos[i].fileinfo = new Table[tableinfos[i].numfiles];
		string filedir = EXTRACT_DIR + filename + PATHSEP;
		cout << "Files being extracted to: " << filedir << '\n';
		FioCreateDirectory(filedir.c_str());
		for (uint j = 0; j < tableinfos[i].numfiles; j++) {
			string tabletext = drstext.substr(tableinfos[i].tbloffset + (j * TABLE_SIZE), TABLE_SIZE);
			tableinfos[i].fileinfo[j].fileid = str2uint(tabletext, 0);
			tableinfos[i].fileinfo[j].fileoffset = str2uint(tabletext, 4);
			tableinfos[i].fileinfo[j].filesize = str2uint(tabletext, 8);

			stringstream ss;
			ss << tableinfos[i].fileinfo[j].fileid;
			string outfilename = filedir;
			outfilename += ss.str();
			outfilename += ".";
			outfilename += tableinfos[i].extension;
			ofstream outputfile;
			outputfile.open(outfilename.c_str(), ios::out | ios::binary);
			if (outputfile.is_open()) {
				outputfile << drstext.substr(tableinfos[i].fileinfo[j].fileoffset, tableinfos[i].fileinfo[j].filesize);
				outputfile.close();
			}
		}
		cout << '\n';
	}
	cout << '\n';
	file.close();
}
