/*
 *  Bz2LineWriter.cpp
 *  Moses Training
 *
 *  Created by Венцислав Жечев on 02.03.11.
 *  © 2011 Венцислав Жечев. All rights reserved.
 *
 */

#include "Bz2LineWriter.h"


namespace bg_zhechev_ventsislav {
	
	Bz2LineWriter::Bz2LineWriter(const string& fileName, bool plain) : bzerror(BZ_OK), uncompressed(plain || fileName == "-" || fileName.find(".bz2") == string::npos), out(NULL), open(true) {
		if (uncompressed) {
//			cerr << "UNCOMPRESSED OUTPUT!!!" << endl;
			if (fileName != "-") {
				uncompressedFile.open(fileName.c_str());
				if (uncompressedFile.fail()) {
					/* handle error */
					wcout << L"Could not open the stupid file for writing!!!" << endl;
					exit(3);
				}
				out = new ostream(uncompressedFile.rdbuf());
			} else
				out = new ostream(cout.rdbuf());
		} else {
//			cerr << "COMPRESSED OUTPUT!!!" << endl;
			plainHandle = fopen(fileName.c_str(), "w");
			if (!plainHandle) {
				/* handle error */
				wcout << L"Could not open the stupid file for writing!!!" << endl;
				exit(5);
			}
			bzip2Handle = BZ2_bzWriteOpen(&bzerror, plainHandle, 9, 0, 0);
			if (bzerror != BZ_OK) {
				BZ2_bzWriteClose(&bzerror, bzip2Handle, 0, NULL, NULL);
				/* handle error */
				wcout << L"Trouble writing to the stupid file!!!" << endl;
				exit(6);
			}
			
			bzerror = BZ_OK;
		}
	}
	
	bool Bz2LineWriter::writeLine(string line) {
		if (uncompressed) {
			*out << line;
//			if (line.find('\n') != string::npos) *out << flush;
			return !(*out).fail();
		} else {
#ifdef __debug_writer__
		//		Debugger();
			wcerr << L"Trying to write a line… (finished is " << finished << L", bzerror is " << bzerror << L")" << endl;
#endif
		
			while (line.size() > bufSize) {
				/* get data to write into buf, and set nBuf appropriately */
				strncpy(buf, line.c_str(), bufSize);
				line = line.substr(bufSize);
			
				BZ2_bzWrite(&bzerror, bzip2Handle, buf, bufSize);
				if (bzerror == BZ_IO_ERROR) {
					BZ2_bzWriteClose(&bzerror, bzip2Handle, 0, NULL, NULL);
					fclose(plainHandle);
					/* handle error */
					wcout << L"Trouble writing the stupid file again!!!" << endl;
					exit(7);
				}
			}
			if (line.size() > 0) {
				strncpy(buf, line.c_str(), line.size());
				
				BZ2_bzWrite(&bzerror, bzip2Handle, buf, (int)line.size());
				if (bzerror == BZ_IO_ERROR) {
					BZ2_bzWriteClose(&bzerror, bzip2Handle, 0, NULL, NULL);
					fclose(plainHandle);
					/* handle error */
					wcout << L"Trouble writing the stupid file again!!!" << endl;
					exit(8);
				}
			}
		
			return true;
		}
	}
	
}