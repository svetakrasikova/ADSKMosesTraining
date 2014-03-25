// $Id: tables-core.cpp 3131 2010-04-13 16:29:55Z pjwilliams $
/*
 *			© 2011 Autodesk Development Sàrl
 *			Last modified by Ventsislav Zhechev on 18 Mar 2011
 */

#include "tables-core.h"

#include "Bz2LineReader.h"

#define UNKNOWNSTR	"UNK"

// as in beamdecoder/tables.cpp
vector<string> tokenize( const char* input ) {
  vector< string > token;
  bool betweenWords = true;
  int start=0;
  int i=0;
  for(; input[i] != '\0'; i++) {
    bool isSpace = (input[i] == ' ' || input[i] == '\t');

    if (!isSpace && betweenWords) {
      start = i;
      betweenWords = false;
    }
    else if (isSpace && !betweenWords) {
      token.push_back( string( input+start, i-start ) );
      betweenWords = true;
    }
  }
  if (!betweenWords)
    token.push_back( string( input+start, i-start ) );
  return token;
}

WORD_ID Vocabulary::storeIfNew(const WORD& word) {
  map<WORD, WORD_ID>::iterator i = lookup.find(word);
  
  if (i != lookup.end())
    return i->second;

  WORD_ID id = vocab.size();
  vocab.push_back(word);
	lookup.insert(i, make_pair(word, id));
  return id;  
}

WORD_ID Vocabulary::getWordID(const WORD& word) {
  map<WORD, WORD_ID>::const_iterator i = lookup.find(word);
  if (i == lookup.end())
    return 0;
	else
		return i->second;
}

PHRASE_ID PhraseTable::storeIfNew(const PHRASE& phrase) {
  map<PHRASE, PHRASE_ID>::iterator i = lookup.find(phrase);
  if(i != lookup.end())
    return i->second;

  PHRASE_ID id  = phraseTable.size();
  phraseTable.push_back(phrase);
	lookup.insert(i, make_pair(phrase, id));
  return id;
}

PHRASE_ID PhraseTable::getPhraseID(const PHRASE& phrase) {
  map<PHRASE, PHRASE_ID>::const_iterator i = lookup.find(phrase);
  if(i == lookup.end())
    return 0;
	else
		return i->second;
}

void PhraseTable::clear() {
  lookup.clear();
  phraseTable.clear();
}

void DTable::init() {
  for(int i = -10; i<10; ++i)
    dtable[i] = -abs(i);
}

void DTable::load(const string& fileName) {
	using namespace bg_zhechev_ventsislav;
	Bz2LineReader inFile(fileName);

	for (int i = 0; ; ++i) {
		string line = inFile.readLine();
		if (line.empty()) break;

		if (line.find(' ') == string::npos) {
      cerr << "line " << i << " in " << fileName << " too short, skipping" << endl;
      continue;
    }
		
		dtable[strtol(line.substr(0, line.find(' ')).c_str(), NULL, 10)] =
				log(strtof(line.substr(line.find(' ') + 1).c_str(), NULL));
  }
	
	inFile.close();
}

double DTable::get(int distortion) {
	map<int, double>::const_iterator i = dtable.find(distortion);
	if (i == dtable.end())
		return 0.00001;
	else
		return i->second;
}

