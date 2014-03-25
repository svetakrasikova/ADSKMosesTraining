/*
 *  Bz2LineReader.h
 *  Moses Training
 *
 *  Created by Венцислав Жечев on 17.10.09.
 *  © 2009–2011 Венцислав Жечев. All rights reserved.
 *
 */

#ifndef BZ2LINEREADER
#define BZ2LINEREADER

#include <cassert>
#include <iostream>
#include <fstream>
#include <string>

#include <bzlib.h>
#include <cerrno>

using namespace std;

namespace bg_zhechev_ventsislav {
	
	class Bz2LineReader {
		FILE* plainHandle;
		BZFILE* bzip2Handle;
		unsigned charsRead;
		static const unsigned bufSize = 100;
		char buf[bufSize];
		int bzerror;
		string line;
		string dataBuf;
		bool finished;
		
		bool uncompressed;
		ifstream uncompressedFile;
		istream *in;
		
		bool open;
		
		//Disable default constructor
		Bz2LineReader() {}

		void processBuffer(char buf[], size_t bufLen);
		bool findLine(bool includeLast = false);
		
		friend string readChunk(istream& input, char delim, bool includeLast);
	public:
		Bz2LineReader(const string& fileName, bool plain = false);
		~Bz2LineReader() { close(); }
		
		inline void close() {
			if (!open) return;
			if (uncompressed) {
				if (in != NULL) {
					delete in;
					if (uncompressedFile.is_open())
						uncompressedFile.close();
				}
			} else {
				BZ2_bzReadClose(&bzerror, bzip2Handle);
				fclose(plainHandle);
			}
			open = false;
		}
		
		string readLine(bool includeLast = false);
		string readLine(unsigned maxLength, bool includeLast = false);

		static const bool UNCOMPRESSED = true;
		static const bool COMPRESSED = false;
	};
	
}

#endif