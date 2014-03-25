/***********************************************************************
  Moses - factored phrase-based language decoder
  © 2009 University of Edinburgh
  © 2011 Autodesk Development Sàrl
 
  Last modified by Ventsislav Zhechev on 18 Mar 2011
  Changes:
  1) Switched to using C++ string instead of C-style char* where possible
  2) Performance optimisations
  3) Switched to reading and writing Bzip2-compressed data to reduce I/O operations
  4) Can be used in a pipe by supplying - for any file name, based on demand
 

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 ***********************************************************************/

#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <cassert>
#include <set>

#include "tables-core.h"

#include "Bz2LineReader.h"
#include "Bz2LineWriter.h"

using namespace std;
using namespace bg_zhechev_ventsislav;

// data structure for a single phrase pair
struct PhraseAlignment {
	int target, source;
	float count;
	vector< set<size_t> > alignedToT;
	vector< set<size_t> > alignedToS;
  
	void create(const vector<string>&, int );
	void addToCount(string);
	inline void clear() { alignedToT.clear(); alignedToS.clear(); }
	inline bool equals(const PhraseAlignment& other) { return (other.target == target && other.source == source && other.alignedToT == alignedToT && other.alignedToS == alignedToS); }
	bool match( const PhraseAlignment& );
};

Vocabulary vcbT;
Vocabulary vcbS;

struct LexicalTable {
  map< WORD_ID, map< WORD_ID, double > > ltable;
  void load( char[] );
  double permissiveLookup( WORD_ID wordS, WORD_ID wordT ) {
		// cout << endl << vcbS.getWord( wordS ) << "-" << vcbT.getWord( wordT ) << ":";
		map<WORD_ID, map<WORD_ID, double> >::const_iterator itS = ltable.find(wordS);
		if (itS == ltable.end()) return 1.0;
		map<WORD_ID, double>::const_iterator itT = itS->second.find(wordT);
		if (itT == itS->second.end()) return 1.0;
		// cout << ltable[ wordS ][ wordT ];
		return itT->second;
  }
};

vector<string> tokenize(const char[]);

void computeCountOfCounts(const string&);
void processPhrasePairs(vector<PhraseAlignment>&, Bz2LineWriter&);
PhraseAlignment* findBestAlignment(vector<PhraseAlignment*>&);
void outputPhrasePair(vector<PhraseAlignment*>&, float, Bz2LineWriter&);
double computeLexicalTranslation(PHRASE&, PHRASE&, PhraseAlignment*);

ofstream wordAlignmentFile;

LexicalTable lexTable;
PhraseTable phraseTableT;
PhraseTable phraseTableS;
bool inverseFlag = false;
bool hierarchicalFlag = false;
bool wordAlignmentFlag = false;
bool onlyDirectFlag = false;
bool goodTuringFlag = false;
#define GT_MAX 10
bool logProbFlag = false;
int negLogProb = 1;
bool lexFlag = true;
int countOfCounts[GT_MAX+1];
float discountFactor[GT_MAX+1];

int main(int argc, char* argv[]) {
	cerr	<< "Score v2.5 written by Philipp Koehn" << endl
				<< "Modified by Ventsislav Zhechev, Autodesk Development Sàrl" << endl
				<< "scoring methods for extracted rules" << endl
	;

	if (argc < 4) {
		cerr << "syntax: score extract lex phrase-table [--Inverse] [--Hierarchical] [--OnlyDirect] [--LogProb] [--NegLogProb] [--NoLex] [--GoodTuring] [--WordAlignment file]\n";
		exit(1);
	}
	char* fileNameExtract = argv[1];
	char* fileNameLex = argv[2];
	char* fileNamePhraseTable = argv[3];
	char* fileNameWordAlignment;

	for(int i=4; i<argc; ++i) {
		if (strcmp(argv[i],"inverse") == 0 || strcmp(argv[i],"--Inverse") == 0) {
			inverseFlag = true;
			cerr << "using inverse mode\n";
		}
		else if (strcmp(argv[i],"--Hierarchical") == 0) {
			hierarchicalFlag = true;
			cerr << "processing hierarchical rules\n";
		}
		else if (strcmp(argv[i],"--OnlyDirect") == 0) {
			onlyDirectFlag = true;
			cerr << "outputing in correct phrase table format (no merging with inverse)\n";
		}
		else if (strcmp(argv[i],"--WordAlignment") == 0) {
			wordAlignmentFlag = true;
			fileNameWordAlignment = argv[++i];
			cerr << "outputing word alignment in file " << fileNameWordAlignment << endl;
		}
		else if (strcmp(argv[i],"--NoLex") == 0) {
			lexFlag = false;
			cerr << "not computing lexical translation score\n";
		}
		else if (strcmp(argv[i],"--GoodTuring") == 0) {
			goodTuringFlag = true;
			cerr << "using Good Turing discounting\n";
		}
		else if (strcmp(argv[i],"--LogProb") == 0) {
			logProbFlag = true;
			cerr << "using log-probabilities\n";
		}
		else if (strcmp(argv[i],"--NegLogProb") == 0) {
			logProbFlag = true;
			negLogProb = -1;
			cerr << "using negative log-probabilities\n";
		}
		else {
			cerr << "ERROR: unknown option " << argv[i] << endl;
			exit(1);
		}
	}

	// lexical translation table
	if (lexFlag)
		lexTable.load(fileNameLex);
  
	// compute count of counts for Good Turing discounting
	if (goodTuringFlag)
		computeCountOfCounts(fileNameExtract);

	// sorted phrase extraction file
	Bz2LineReader extractFile(fileNameExtract);

	// output file: phrase translation table
	Bz2LineWriter phraseTableFile(fileNamePhraseTable);

	// output word alignment file
	if (!inverseFlag && wordAlignmentFlag) {
		wordAlignmentFile.open(fileNameWordAlignment);
		if (wordAlignmentFile.fail()) {
			cerr << "ERROR: could not open word alignment file "
			     << fileNameWordAlignment << endl;
			exit(1);
		}
	}
  
  // loop through all extracted phrase translations
  int lastSource = -1;
  vector< PhraseAlignment > phrasePairsWithSameF;
  int i=0;
	string lastLine = "";
	PhraseAlignment *lastPhrasePair = NULL;
	for (string line = extractFile.readLine(); !line.empty(); line = extractFile.readLine()) {
		if (line.empty()) break;
		if ((++i)%10000000 == 0) cerr << "[p. score:" << i << "]" << flush;
    else if (i % 100000 == 0) cerr << "." << flush;
		
		// identical to last line? just add count
		if (lastSource >= 0 && line == lastLine) {
			lastPhrasePair->addToCount(line);
			continue;
		}
		lastLine = line;

		// create new phrase pair
		PhraseAlignment phrasePair;
		vector<string> lineVector = tokenize(line.c_str());
		phrasePair.create(lineVector, i);
		
		// only differs in count? just add count
		if (lastPhrasePair != NULL && lastPhrasePair->equals(phrasePair)) {
			lastPhrasePair->count += phrasePair.count;
			phrasePair.clear();
			continue;
		}
		
		// if new source phrase, process last batch
		if (lastSource >= 0 && lastSource != phrasePair.source) {
			processPhrasePairs(phrasePairsWithSameF, phraseTableFile);
			for (size_t j=0; j<phrasePairsWithSameF.size(); phrasePairsWithSameF[j++].clear());
			phrasePairsWithSameF.clear();
			phraseTableT.clear();
			phraseTableS.clear();
			// process line again, since phrase tables flushed
			phrasePair.clear();
			phrasePair.create(lineVector, i);
		}
		
		// add phrase pairs to list, it's now the last one
		lastSource = phrasePair.source;
		phrasePairsWithSameF.push_back(phrasePair);
		lastPhrasePair = &phrasePairsWithSameF[phrasePairsWithSameF.size()-1];
	}
	processPhrasePairs(phrasePairsWithSameF, phraseTableFile);
	if (!inverseFlag && wordAlignmentFlag)
		wordAlignmentFile.close();
}

void computeCountOfCounts(const string& fileNameExtract) {
	if (fileNameExtract == "-") {
		cerr << "The ‘GoodTuring Discounting’ option may not be used with piped input!" << endl;
		exit(9);
	}

	cerr << "computing counts of counts";
	for (size_t i=1; i<=GT_MAX; countOfCounts[i++] = 0);

	Bz2LineReader extractFile(fileNameExtract);

	// loop through all extracted phrase translations
	int i=0;
	string lastLine;
	PhraseAlignment *lastPhrasePair = NULL;

	for (string line = extractFile.readLine(); !line.empty(); line = extractFile.readLine()) {
		if (line.empty()) break;
		if ((++i)%10000000 == 0) cerr << "[" << i << "]" << endl;
    else if (i % 100000 == 0) cerr << "," << flush;
		
		// identical to last line? just add count
		if (line == lastLine) {
			lastPhrasePair->addToCount(line);
			continue;			
		}
		lastLine = line;

		// create new phrase pair
		PhraseAlignment *phrasePair = new PhraseAlignment();
		vector<string> lineVector = tokenize(line.c_str());
		phrasePair->create(lineVector, i);
		
		if (i == 1) {
			lastPhrasePair = phrasePair;
			continue;
		}

		// only differs in count? just add count
		if (lastPhrasePair->match( *phrasePair )) {
			lastPhrasePair->count += phrasePair->count;
			phrasePair->clear();
			delete(phrasePair);
			continue;
		}

		// periodically house cleaning
		if (phrasePair->source != lastPhrasePair->source) {
			phraseTableT.clear(); // these would get too big
			phraseTableS.clear(); // these would get too big
			// process line again, since phrase tables flushed
			phrasePair->clear();
			phrasePair->create(lineVector, i); 
		}

		int count = lastPhrasePair->count + 0.99999;
		if(count <= GT_MAX)
			++countOfCounts[ count ];
		lastPhrasePair->clear();
		delete( lastPhrasePair );
		lastPhrasePair = phrasePair;
	}

	discountFactor[0] = 0.01; // floor
	cerr << "\n";
	for(int i=1;i<GT_MAX; ++i) {
		discountFactor[i] = ((float)i+1)/(float)i*(((float)countOfCounts[i+1]+0.1) / ((float)countOfCounts[i]+0.1));
		cerr << "count " << i << ": " << countOfCounts[ i ] << ", discount factor: " << discountFactor[i];
		// some smoothing...
		if (discountFactor[i]>1) 
			discountFactor[i] = 1;
		if (discountFactor[i]<discountFactor[i-1])
			discountFactor[i] = discountFactor[i-1];
		cerr << " -> " << discountFactor[i]*i << endl;
	}
}

bool isNonTerminal( string &word ) 
{
	return (word.length()>=3 &&
		word.substr(0,1).compare("[") == 0 && 
		word.substr(word.length()-1,1).compare("]") == 0);
}
	
void processPhrasePairs(vector<PhraseAlignment>& phrasePair, Bz2LineWriter& phraseTableFile) {
  if (phrasePair.size() == 0) return;

	// group phrase pairs based on alignments that matter
	// (i.e. that re-arrange non-terminals)
	vector<vector<PhraseAlignment*> > phrasePairGroup;
	float totalSource = 0.;
	
	// loop through phrase pairs
	for(size_t i=0; i<phrasePair.size(); ++i) {
		// add to total count
		totalSource += phrasePair[i].count;

		bool matched = false;
		// check for matches
		for(size_t g=0; g<phrasePairGroup.size(); ++g) {
			vector< PhraseAlignment* > &group = phrasePairGroup[g];
			// matched? place into same group
			if ( group[0]->match( phrasePair[i] )) {
				group.push_back( &phrasePair[i] );
				matched = true;
			}
		}
		// not matched? create new group
		if (!matched) {
			vector< PhraseAlignment* > newGroup;
			newGroup.push_back( &phrasePair[i] );
			phrasePairGroup.push_back( newGroup );
		}
	}
	
	for(size_t g=0; g<phrasePairGroup.size(); ++g) {
		vector< PhraseAlignment* > &group = phrasePairGroup[g];
		outputPhrasePair(group, totalSource, phraseTableFile);
	}
}

PhraseAlignment* findBestAlignment( vector< PhraseAlignment* > &phrasePair ) {
	float bestAlignmentCount = -1.;
	PhraseAlignment* bestAlignment;

	for(size_t i=0; i<phrasePair.size(); ++i)
		if (phrasePair[i]->count > bestAlignmentCount) {
			bestAlignmentCount = phrasePair[i]->count;
			bestAlignment = phrasePair[i];
		}
	
	return bestAlignment;
}

void outputPhrasePair(vector<PhraseAlignment*> &phrasePair, float totalCount, Bz2LineWriter& phraseTableFile) {
  if (phrasePair.size() == 0)
		return;

	PhraseAlignment *bestAlignment = findBestAlignment( phrasePair );

	// compute count
	float count = 0.;
	for(size_t i=0; i<phrasePair.size(); count += phrasePair[i++]->count);

	PHRASE phraseS = phraseTableS.getPhrase( phrasePair[0]->source );
	PHRASE phraseT = phraseTableT.getPhrase( phrasePair[0]->target );

	// labels (if hierarchical)

	// source phrase (unless inverse)
	if (!inverseFlag) {
		for (size_t j=0; j<phraseS.size(); phraseTableFile.writeLine(vcbS.getWord(phraseS[j++]) + " "));
		phraseTableFile.writeLine("||| ");
	}
	
	// target phrase
	for (size_t j=0; j<phraseT.size(); phraseTableFile.writeLine(vcbT.getWord(phraseT[j++]) + " "));
	phraseTableFile.writeLine("||| ");
	
	// source phrase (if inverse)
	if (inverseFlag) {
		for (size_t j=0; j<phraseS.size(); phraseTableFile.writeLine(vcbS.getWord(phraseS[j++]) + " "));
		phraseTableFile.writeLine("||| ");
	}
	
	// alignment info for non-terminals
	if (!inverseFlag && hierarchicalFlag) {
    assert(phraseT.size() == bestAlignment->alignedToT.size() + 1);
		for(size_t j = 0; j < phraseT.size() - 1; ++j)
			if (isNonTerminal(vcbT.getWord( phraseT[j] ))) {
        assert(bestAlignment->alignedToT[ j ].size() == 1);
				stringstream data;
				data << *(bestAlignment->alignedToT[j].begin()) << "-" << j << " ";
				phraseTableFile.writeLine(data.str());
			}
		phraseTableFile.writeLine("||| ");
	}

	// phrase translation probability
	if (goodTuringFlag && count<GT_MAX)
		count *= discountFactor[(int)(count+0.99999)];
	
	{
		stringstream data;
		data << (logProbFlag ? negLogProb*log(count / totalCount) : count / totalCount);
		phraseTableFile.writeLine(data.str());
	}
	
	// lexical translation probability
	if (lexFlag) {
		stringstream data;
		data << " " << (logProbFlag ?
										negLogProb*log(computeLexicalTranslation(phraseS, phraseT, bestAlignment)) :
										computeLexicalTranslation(phraseS, phraseT, bestAlignment));
		phraseTableFile.writeLine(data.str());
	}

	{
		stringstream data;
		data << " ||| " << totalCount << endl;
		phraseTableFile.writeLine(data.str());
	}

	// optional output of word alignments
	if (!inverseFlag && wordAlignmentFlag) {
		// source phrase
		for(size_t j=0;j<phraseS.size(); wordAlignmentFile << vcbS.getWord(phraseS[j++]) << " ");
		wordAlignmentFile << "||| ";
	
		// target phrase
		for(size_t j=0;j<phraseT.size(); wordAlignmentFile << vcbT.getWord(phraseT[j++]) << " ");
		wordAlignmentFile << "|||";

		// alignment
		for(size_t j=0;j<bestAlignment->alignedToT.size(); ++j) {
			const set< size_t > &aligned = bestAlignment->alignedToT[j];
      for (set< size_t >::const_iterator p(aligned.begin()); p != aligned.end(); wordAlignmentFile << " " << *(p++) << "-" << j);
		}
		wordAlignmentFile << endl;
	}
}

double computeLexicalTranslation( PHRASE &phraseS, PHRASE &phraseT, PhraseAlignment *alignment ) {
	// lexical translation probability
	double lexScore = 1.;
	int null = vcbS.getWordID("NULL");
	// all target words have to be explained
	for (size_t ti=0; ti<alignment->alignedToT.size(); ++ti) { 
    const set<size_t>& srcIndices = alignment->alignedToT[ti];
		if (srcIndices.empty())
		// explain unaligned word by NULL
			lexScore *= lexTable.permissiveLookup(null, phraseT[ti]); 
		else {
		 // go through all the aligned words to compute average
			double thisWordScore = 0.;
      for (set<size_t>::const_iterator p(srcIndices.begin()); p != srcIndices.end(); thisWordScore += lexTable.permissiveLookup(phraseS[*(p++)], phraseT[ti]));
			lexScore *= (thisWordScore / srcIndices.size());
		}
	}
	return lexScore;
}

void PhraseAlignment::addToCount(string line) {
	unsigned short items = 1;
	while (line.find("|||") != string::npos) {
		line = line.substr(line.find("|||") + 3);
		++items;
	}
	if (items == 3) // no specified counts -> counts as one
		count += 1.;
	else
		count += strtof(line.c_str(), NULL);
}

// read in a phrase pair and store it
void PhraseAlignment::create(const vector<string>& token, int lineID) {
	int item = 1;
	PHRASE phraseS, phraseT;
	for (size_t j=0; j<token.size(); ++j) {
		if (token[j] == "|||")
			item++;
		else if (item == 1) // source phrase
			phraseS.push_back( vcbS.storeIfNew( token[j] ) );
		else if (item == 2) // target phrase
			phraseT.push_back( vcbT.storeIfNew( token[j] ) );
		else if (item == 3) { // alignment
			int s = strtol(token[j].substr(0, token[j].find("-")).c_str(), NULL, 10);
			int t = strtol(token[j].substr(token[j].find("-") + 1).c_str(), NULL, 10);
			if (t >= phraseT.size() || s >= phraseS.size()) {
				cerr << "WARNING: phrase pair " << lineID 
						 << " has alignment point (" << s << ", " << t 
						 << ") out of bounds (" << phraseS.size() << ", " << phraseT.size() << ")\n";
			} else {
				// first alignment point? -> initialize
				if (alignedToT.size() == 0) {
          assert(alignedToS.size() == 0);
          size_t numTgtSymbols = (hierarchicalFlag ? phraseT.size()-1 : phraseT.size());
          alignedToT.resize(numTgtSymbols);
          size_t numSrcSymbols = (hierarchicalFlag ? phraseS.size()-1 : phraseS.size());
          alignedToS.resize(numSrcSymbols);
					source = phraseTableS.storeIfNew( phraseS );
					target = phraseTableT.storeIfNew( phraseT );
				}
				// add alignment point
				alignedToT[t].insert( s );
				alignedToS[s].insert( t );
			}
		} else if (item == 4) // count
			count = strtof(token[j].c_str(), NULL);
	}
	if (item == 3)
		count = 1.0;
	if (item < 3 || item > 4) {
		cerr << "ERROR: faulty line " << lineID << ": ";
		for(vector<string>::const_iterator i = token.begin(); i != token.end(); cerr << *(i++) << " ");
		cerr << endl;
	}
}

// check if two word alignments between a phrase pairs "match"
// i.e. they do not differ in the alignment of non-termimals
bool PhraseAlignment::match( const PhraseAlignment& other )
{
	if (other.target != target || other.source != source) return false;
	if (!hierarchicalFlag) return true;

	PHRASE phraseT = phraseTableT.getPhrase( target );

  assert(phraseT.size() == alignedToT.size() + 1);
  assert(alignedToT.size() == other.alignedToT.size());

	// loop over all words (note: 0 = left hand side of rule)
	for(size_t i=0;i<phraseT.size()-1;++i)
		if (isNonTerminal( vcbT.getWord( phraseT[i] ) )) {
			if (alignedToT[i].size() != 1 ||
			    other.alignedToT[i].size() != 1 ||
		    	    *(alignedToT[i].begin()) != *(other.alignedToT[i].begin()))
				return false;
		}

	return true;
}


void LexicalTable::load(char *fileName) {
  cerr << "Loading lexical translation table from " << fileName;
	Bz2LineReader inFile(fileName, Bz2LineReader::UNCOMPRESSED);
	
	int i = 0;
	for (string line = inFile.readLine(); !line.empty(); line = inFile.readLine()) {
		if (line.empty())
			break;
    if (++i%100000 == 0) cerr << "." << flush;
		
    vector<string> token = tokenize(line.c_str());
    if (token.size() != 3) {
      cerr << "line " << i << " “" << line << "” in " << fileName 
			     << " has wrong number of tokens (" << token.size() << "), skipping:\n"
			     << token.size() << " " << token[0] << " " << line << endl;
      continue;
    }
  
    WORD_ID wordT = vcbT.storeIfNew( token[0] );
    WORD_ID wordS = vcbS.storeIfNew( token[1] );
    ltable[ wordS ][ wordT ] = strtod(token[2].c_str(), NULL);
  }
  cerr << endl;
}

