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

#include "RAF.h"

#include <direct.h>
#include <zlib.h>

//TODO remove
#include <iostream>
using namespace std;

#define FS_DELIMITERS "\\/"
#define FS_DEFAULT_DELIMITER "/"

void RAF::open(string file_path) {
	raf_file.open(file_path.c_str(), ios::binary | ios::ate);
	data_file.open((file_path + ".dat").c_str(), ios::binary);

	if(!raf_file.is_open()) {
		cout << "failed to open " << file_path << endl;
		//TODO throw
	}

	if(!data_file.is_open()) {
		cout << "failed to open " << file_path << endl;
		//TODO throw
	}

	raf_file_size = raf_file.tellg();
}

void RAF::read() {
	//TODO handle I/O errors etc

	// skip constant 16 byte header
	raf_file.seekg(16);

	// block 1
	raf_file.read((char*) &block1.offset_block2, 4);
	raf_file.read((char*) &block1.num_records, 4);
	block1.records = new block1::record[block1.num_records];
	raf_file.read((char*) block1.records, block1.num_records * sizeof(block1::record));

	// block 2
	raf_file.read((char*) &block2.offset_unknown, 4);
	raf_file.read((char*) &block2.num_records, 4);
	block2.records = new block2::record[block2.num_records];
	raf_file.read((char*) block2.records, block2.num_records * sizeof(block2::record));

	//TODO
	// block 1 & 2 consistency check
	//if(block1.num_records != block2.num_records)
		// ...

	// block 3
	//TODO nothing else at end of file?
	pos_type bytes_left = raf_file_size - raf_file.tellg();
	block3.filenames = new char[bytes_left];
	raf_file.read((char*) block3.filenames, bytes_left);
}

void RAF::unpack(string target_path) {
	char* filename_ptr = block3.filenames;

	for(raf_int i = 0; i < block2.num_records; i++) {
		bool block1_record_found = false;

		for(raf_int j = 0; j < block1.num_records; j++) {
			if(block1.records[j].index == i) {
				extract_file(target_path,
							 filename_ptr,
							 block1.records[j].dat_file_offset,
							 block1.records[j].dat_file_size);
				block1_record_found = true;
				break;
			}
		}

		//TODO proper error handling
		if(!block1_record_found)
			cout << "invalid record index for file: " << filename_ptr << endl;

		// increment filename pointer
		filename_ptr += block2.records[i].filename_size;
	}
}

void RAF::extract_file(string target_path, string file_name, raf_int data_offset, raf_int data_size) {
	cout << "extracting " << file_name << endl;

	byte* raw_buffer = new byte[data_size];

	//TODO handle potential errors

	//TODO extract directory creation into member function
	string target_file_path = target_path + file_name;
	char target_file_path_cstr[target_file_path.length() + 1];
	target_file_path.copy(target_file_path_cstr, target_file_path.length(), 0);

	char* token = NULL;
	char* last_token = strtok(target_file_path_cstr, FS_DELIMITERS);
	string path_created = "";

	while((token = strtok(NULL, FS_DELIMITERS)) != NULL) {
		path_created += last_token;
		path_created += FS_DEFAULT_DELIMITER;

		_mkdir(path_created.c_str());

		last_token = token;
	}

	// extract

	//TODO handle errors!
	ofstream target_file;
	target_file.open(target_file_path.c_str(), ios::binary | ios::trunc);

	data_file.seekg(data_offset);
	data_file.read((char*) raw_buffer, data_size);

	const unsigned int decompress_buffer_size = data_size * 2;
	byte* decompress_buffer = new byte[decompress_buffer_size];

	// decompress

	z_stream zlib_stream;
	zlib_stream.next_in 	= (Bytef*) raw_buffer;
	zlib_stream.avail_in	= data_size;
	zlib_stream.next_out	= (Bytef*) decompress_buffer;
	zlib_stream.avail_out	= decompress_buffer_size;
	zlib_stream.zalloc 		= Z_NULL;
	zlib_stream.zfree 		= Z_NULL;
	zlib_stream.opaque 		= Z_NULL;

	// throw
	if(inflateInit(&zlib_stream) != Z_OK) {
		cout << "failed to initialize zlib stream for inflation" << endl;
		return;
	}

	int inflate_retval;

	while(zlib_stream.total_in < data_size) {
		inflate_retval = inflate(&zlib_stream, Z_BLOCK);

		switch(inflate_retval) {
		case Z_OK:
		case Z_STREAM_END:
			break;
		case Z_NEED_DICT:
			cout << "inflate reported an error: Z_NEED_DICT" << endl;
			return;
		case Z_DATA_ERROR:
			cout << "inflate reported an error: Z_DATA_ERROR" << endl;
			return;
		case Z_STREAM_ERROR:
			cout << "inflate reported an error: Z_STREAM_ERROR" << endl;
			return;
		case Z_MEM_ERROR:
			cout << "inflate reported an error: Z_STREAM_ERROR" << endl;
			return;
		case Z_BUF_ERROR:
			cout << "inflate reported an error: Z_BUF_ERROR" << endl;
			return;
		default:
			cout << "inflate reported an unexpected error: " << endl;
			return;
		}

		target_file.write((char*) decompress_buffer, decompress_buffer_size - zlib_stream.avail_out);
		zlib_stream.next_out = (Bytef*) decompress_buffer;
		zlib_stream.avail_out = decompress_buffer_size;

		//TODO handle hypothetical error case: next_in has become 0 while total_in < data_size
	}

	//TODO proper error handling
	// this should never happen
	if(inflate_retval != Z_STREAM_END) {
		cout << "unexpected error" << endl;
		return;
	}

	//TODO do the following in finally { ... }

	//TODO proper error handling
	if(inflateEnd(&zlib_stream) != Z_OK) {
		cout << "failed to destroy zlib stream" << endl;
		return;
	}

	delete[] raw_buffer;
	delete[] decompress_buffer;
	target_file.close();
}
