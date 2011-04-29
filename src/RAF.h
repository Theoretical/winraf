/*
 * Copyright © 2011 Martin Riedel
 *
 * This file is part of winraf.
 *
 * winraf is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * winraf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with winraf.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef RAF_H_
#define RAF_H_

#include <fstream>
#include <string>

using std::ifstream;
using std::string;

typedef ifstream::pos_type pos_type;
typedef unsigned int raf_int;
typedef unsigned char byte;

class RAF {

public:

	RAF(string file_path) {
		open(file_path);
		read();
	}

	~RAF() {
		if(block1.records != NULL)
			delete[] block1.records;
		if(block2.records != NULL)
			delete[] block2.records;
		if(block3.filenames != NULL)
			delete[] block3.filenames;

		if(raf_file.is_open())
			raf_file.close();
		if(data_file.is_open())
			data_file.close();
	}

	void unpack(string target_path);

private:

	ifstream raf_file;
	ifstream data_file;
	pos_type raf_file_size;

	struct block1 {
		struct record {
			raf_int unknown;			// 4 bytes
			raf_int dat_file_offset;	// 4 bytes
			raf_int	dat_file_size;		// 4 bytes
			raf_int	index;				// 4 bytes
		};

		raf_int	offset_block2;	// 4 bytes
		raf_int num_records;	// 4 bytes
 		record*	records;		// num_records * 16 bytes
	} block1;

	struct block2 {
		struct record {
			raf_int	filename_offset;	// 4 bytes, offset to file name in block3
			raf_int	filename_size;		// 4 bytes
		};

		raf_int	offset_unknown; // 4 bytes; possibly offset to block3
		raf_int	num_records;	// 4 bytes
		record*	records;		// num_records * 8 bytes
	} block2;

	struct block3 {
		char*	filenames;	// zero-delimited filenames
	} block3;

	void open(string file_path);
	void read();
	void extract_file(string target_path, string file_name, raf_int data_offset, raf_int data_size);

};

#endif /* RAF_H_ */
