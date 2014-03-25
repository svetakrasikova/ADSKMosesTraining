/*
 *  Bz2LineReader.cpp
 *  Moses Training
 *
 *  Created by Венцислав Жечев on 17.10.09.
 *  © 2009–2011 Венцислав Жечев. All rights reserved.
 *  © 2012 Autodesk Development Sàrl. All rights reserved.
 *
 */

#include "Bz2LineReader.h"

namespace bg_zhechev_ventsislav {
	string readChunk(istream& input, char delim = '\n', bool includeLast = false) {
		if (input.fail())
			return "";
		
		string output = "";
		
		char lastChar = '\0';
		if (input.good())
			lastChar = input.peek();
		if (lastChar == delim) {
			input.get();
			if (includeLast)
				output += lastChar;
			return output;
		}
		
		char ln[Bz2LineReader::bufSize];
		input.get(ln, Bz2LineReader::bufSize, delim);
		if (input.fail())
			return "";
		output += ln;
		if (input.gcount() <= Bz2LineReader::bufSize && input.good())
			lastChar = input.peek();
#ifdef __debug_chunk__
		cerr << "??? “" << lastChar << "” ??? output: “" << output << "”" << endl;
#endif
		while (lastChar != delim && input.good()) {
			input.get(ln, Bz2LineReader::bufSize, delim);
			if (input.fail()) {
				if (includeLast) {
					return output + delim;
				} else {
					return output;
				}
			}
			output += ln;
			if (input.gcount() <= Bz2LineReader::bufSize && input.good())
				lastChar = input.peek();
#ifdef __debug_chunk__
			cerr << "??? “" << lastChar << "” ??? output: “" << output << "”" << endl;
#endif
		}
		
#ifdef __debug_chunk__
		if (!input.good())
			wcerr << L"÷÷÷Terminated input loop due to bad input!!!÷÷÷" << endl;
		else
			wcerr << L"±±±Terminated input loop due to reaching delimiting character.±±±" << endl;
#endif
		
		if (input.good() && lastChar == delim) {
			if (includeLast)
				output += lastChar;
			input.get();
		}
		
#ifdef __debug_chunk__
		cerr << " " << output << endl;
#endif
		
		return output;
	}
	
	void Bz2LineReader::processBuffer(char buf[], size_t bufLen) {
		for (size_t i = 0; i < bufLen; dataBuf += buf[i++]);
	}
	
	bool Bz2LineReader::findLine(bool includeLast) {
		string::size_type lineBreak = dataBuf.find('\n');
		if (lineBreak == string::npos) {
			line += dataBuf;
			dataBuf.clear();
			return false;
		} else {
			line += dataBuf.substr(0, lineBreak + (includeLast ? 1 : 0));
			dataBuf = dataBuf.substr(lineBreak + 1);
			return true;
		}
	}
		
	Bz2LineReader::Bz2LineReader(const string& fileName, bool plain) : charsRead(0), bzerror(BZ_OK), line(string()), finished(false), uncompressed(plain || fileName == "-" || fileName.find(".bz2") == string::npos), in(NULL), dataBuf(string()), open(true) {
		if (uncompressed) {
//			cerr << "UNCOMPRESSED INPUT!!!" << endl;
			if (fileName != "-") {
				uncompressedFile.open(fileName.c_str());
				if (uncompressedFile.fail()) {
					/* handle error */
					wcout << L"Could not open the stupid file!!!" << endl;
					exit(3);
				}
				in = new istream(uncompressedFile.rdbuf());
			} else
				in = new istream(cin.rdbuf());
		} else {
//			cerr << "COMPRESSED INPUT!!!" << endl;
			plainHandle = fopen(fileName.c_str(), "r");
			if (!plainHandle) {
				/* handle error */
				wcout << L"Could not open the stupid file!!!" << endl;
				exit(5);
			}
			bzip2Handle = BZ2_bzReadOpen(&bzerror, plainHandle, 1, 0, NULL, 0);
			if (bzerror != BZ_OK) {
				BZ2_bzReadClose(&bzerror, bzip2Handle);
				/* handle error */
				wcout << L"Trouble reading the stupid file!!!" << endl;
				exit(6);
			}
			
			bzerror = BZ_OK;
		}
	}
	
	string Bz2LineReader::readLine(bool includeLast) {
		if (uncompressed)
			return readChunk(*in, '\n', includeLast);
		else {
			
			assert (!finished || bzerror != BZ_STREAM_END);
		
#ifdef __debug_reader__
//		Debugger();
			wcerr << L"Trying to get a line… (finished is " << finished << L", bzerror is " << bzerror << L")" << endl;
#endif
		
			if (findLine(includeLast)) {
#ifdef __debug_reader__
				wcerr << L"±±Apparently there was a line in the local buffer." << endl;
#endif
				string out = line;
				line = "";
				return out;
			}

			if (!finished) {
				if (bzerror == BZ_OK) {
#ifdef __debug_reader__
					wcerr << L"Getting more data from compressed file…" << endl;
#endif
					while (bzerror == BZ_OK) {
						charsRead = BZ2_bzRead(&bzerror, bzip2Handle, buf, bufSize);
						if (bzerror == BZ_OK) {
							processBuffer(buf, charsRead);
							if (findLine(includeLast)) {
								string out = line;
								line = "";
								return out;
							}
						}
					}
					if (bzerror != BZ_STREAM_END) {
						BZ2_bzReadClose(&bzerror, bzip2Handle);
						/* handle error */
						wcout << L"Trouble reading the stupid file again!!!" << endl;
						exit(7);
					}
				}
				if (bzerror == BZ_STREAM_END) {
#ifdef __debug_reader__
					wcerr << L"Finishing up reading from compressed file…" << endl;
#endif
					bzerror = BZ_OK;
					processBuffer(buf, charsRead);
					finished = true;
					if (findLine(includeLast)) {
						string out = line;
						line = "";
						return out;
					}
				}
			}
#ifdef __debug_reader__
			wcerr << L"   Aparently there is no more data in the compressed file." << endl;
#endif
		
			return "";
		}
	}
	
	string Bz2LineReader::readLine(unsigned maxLength, bool includeLast) {
		string out(readLine(), includeLast);
		if (out.length() > maxLength)
			return out.substr(0, maxLength);
		else
			return out;
	}
	
}