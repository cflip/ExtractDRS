/*
 * This file is part of ExtractDRS.
 * ExtractDRS is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * ExtractDRS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with ExtractDRS. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file drs.cpp Functions related to extracting .drs files */

#include <cassert>
#include <fstream>

#include "drs.h"
#include "filereader.h"

void DRSFile::ReadHeader(BinaryFileReader &bfr)
{
	this->copyright    = bfr.ReadString(40);
	this->version      = bfr.ReadString( 4);
	this->type         = bfr.ReadString(12);
	this->num_tables   = bfr.ReadNum<int>();
	this->first_offset = bfr.ReadNum<int>();
}

DRSTableInfo DRSFile::ReadTableInfo(BinaryFileReader &bfr)
{
	DRSTableInfo dti;
	dti.character    = bfr.ReadNum<uint8>();
	dti.extension    = bfr.ReadString(3);

	/* Extension is reversed, for whatever reason. */
	std::swap(dti.extension[0], dti.extension[2]);

	dti.table_offset = bfr.ReadNum<int>();
	dti.num_files    = bfr.ReadNum<int>();
	return dti;
}

DRSTable DRSFile::ReadTable(BinaryFileReader &bfr)
{
	DRSTable dt;
	dt.file_id     = bfr.ReadNum<int>();
	dt.file_offset = bfr.ReadNum<int>();
	dt.file_size   = bfr.ReadNum<int>();
	return dt;
}

/**
 * Actually extract the drs file
 * @param path The path to the drs file
 */
void ExtractDRSFile(const std::string &path)
{
    int dirstartpos = path.rfind(PATHSEP) + 1;
	std::string filename = path.substr(dirstartpos, path.length() - dirstartpos);
	std::cout << "Reading " << path << ":\n";

	BinaryFileReader binfile(path);
	DRSFile drsfile;
	std::string filedir = EXTRACT_DIR + filename.substr(0, filename.length() - 4) + PATHSEP;
	std::cout << "Files being extracted to: " << filedir << std::endl;
	GenCreateDirectory(filedir);

	if (binfile.GetRemaining() < HEADER_SIZE) {
		std::cerr << "File is too small: Only " << binfile.GetRemaining() << " bytes long\n";
		return;
	}

	drsfile.ReadHeader(binfile);

	for (int i = 0; i < drsfile.num_tables; i++) {
		drsfile.infos.push_back(drsfile.ReadTableInfo(binfile));
	}

	for (int i = 0; i < drsfile.num_tables; i++) {
		for (int j = 0; j < drsfile.infos[i].num_files; j++) {
			assert((int)binfile.GetPosition() == drsfile.infos[i].table_offset + j * TABLE_SIZE);

			drsfile.infos[i].file_infos.push_back(drsfile.ReadTable(binfile));
		}
	}

	for (int i = 0; i < drsfile.num_tables; i++) {
		for (int j = 0; j < drsfile.infos[i].num_files; j++) {
			assert((int)binfile.GetPosition() == drsfile.infos[i].file_infos[j].file_offset);

			std::string out_file = filedir;
			out_file += std::to_string(drsfile.infos[i].file_infos[j].file_id);
			out_file += '.';
			out_file += drsfile.infos[i].extension;
			std::ofstream out_fs(out_file, std::ios::binary);
			if (!out_fs.is_open()) {
				std::cerr << "Error writing to " << out_file << std::endl;
				/* TODO: Abort instead? */
				binfile.SkipBytes(drsfile.infos[i].file_infos[j].file_size);
				continue;
			}
			binfile.ReadBlob(out_fs, drsfile.infos[i].file_infos[j].file_size);
			out_fs.close();
			if (drsfile.infos[i].extension == "slp") {
				ExtractSLPFile(out_file);
			}
		}
	}
}
