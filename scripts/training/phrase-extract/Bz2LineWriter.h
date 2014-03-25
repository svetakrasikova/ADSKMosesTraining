/*
 *  Bz2LineWriter.h
 *  Moses Training
 *
 *  Created by Венцислав Жечев on 02.03.11.
 *  © 2011 Венцислав Жечев. All rights reserved.
 *
 */

#ifndef BZ2LINEWRITER
#define BZ2LINEWRITER

#include <cassert>
#include <iostream>
#include <fstream>
#include <string>

#include <bzlib.h>
#include <cerrno>

using namespace std;

namespace bg_zhechev_ventsislav {
	
	class Bz2LineWriter {
		FILE* plainHandle;
		BZFILE* bzip2Handle;
		static const unsigned bufSize = 100;
		char buf[bufSize];
		int bzerror;
		
		bool uncompressed;
		ofstream uncompressedFile;
		ostream *out;
		
		bool open;

		//Disable the default constructor
		Bz2LineWriter() {}
	public:

		Bz2LineWriter(const string& fileName, bool plain = false);
		~Bz2LineWriter() { close(); }
		
		inline void close() {
			if (!open) return;
			if (uncompressed) {
				if (out != NULL) {
					delete out;
					if (uncompressedFile.is_open())
						uncompressedFile.close();
				}
			} else {
				if (bzerror != BZ_OK) {
					cerr << "±±± There is a problem with the Bzip2 stream just before the attempt to close it!!!" << endl;
				}
				BZ2_bzWriteClose(&bzerror, bzip2Handle, 0, NULL, NULL);
				fclose(plainHandle);
			}
			open = false;
		}
		
		bool writeLine(string line);

		static const bool UNCOMPRESSED = true;
		static const bool COMPRESSED = false;
};
	
}

#endif