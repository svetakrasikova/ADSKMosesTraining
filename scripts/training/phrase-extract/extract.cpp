/*
 * extract.cpp
 *
 *      Modified by: (C) Nadi Tomeh - LIMSI/CNRS
 *      Machine Translation Marathon 2010, Dublin
 *
 *
 *			©2011 Autodesk Development Sàrl
 *			Last modified by Ventsislav Zhechev on 18 Aug 2011
 *			Changes:
 *			v2.2
 *			All extract output is now compressed on the fly using bzip2
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cassert>

#include <map>
#include <set>
#include <vector>

#include "SentenceAlignment.h"
#include "tables-core.h"

#include "Bz2LineReader.h"
#include "Bz2LineWriter.h"


using namespace std;
using namespace bg_zhechev_ventsislav;

const string extract_version = "v2.2";

// HPhraseVertex represents a point in the alignment matrix
typedef pair<int, int> HPhraseVertex;

// Phrase represents a bi-phrase; each bi-phrase is defined by two points in the alignment matrix:
// bottom-left and top-right
typedef pair<HPhraseVertex, HPhraseVertex> HPhrase;

// HPhraseVector is a vector of HPhrases
typedef vector<HPhrase> HPhraseVector;

// SentenceVertices represents, from all extracted phrases, all vertices that have the same positioning
// The key of the map is the English index and the value is a set of the source ones
typedef map<int, set<int> > HSentenceVertices;

enum REO_MODEL_TYPE {REO_MSD, REO_MSLR, REO_MONO};
enum REO_POS {LEFT, RIGHT, DLEFT, DRIGHT, UNKNOWN};

REO_POS getOrientWordModel(SentenceAlignment &, REO_MODEL_TYPE, bool, bool,
													 int, int, int, int, int, int, int,
													 bool (*)(int, int), bool (*)(int, int));
REO_POS getOrientPhraseModel(SentenceAlignment &, REO_MODEL_TYPE, bool, bool,
														 int, int, int, int, int, int, int,
														 bool (*)(int, int), bool (*)(int, int),
														 const HSentenceVertices &, const HSentenceVertices &);
REO_POS getOrientHierModel(SentenceAlignment &, REO_MODEL_TYPE, bool, bool,
													 int, int, int, int, int, int, int,
													 bool (*)(int, int), bool (*)(int, int),
													 const HSentenceVertices &, const HSentenceVertices &,
													 const HSentenceVertices &, const HSentenceVertices &,
													 REO_POS);

void insertVertex(HSentenceVertices &, int, int);
void insertPhraseVertices(HSentenceVertices &, HSentenceVertices &, HSentenceVertices &, HSentenceVertices &,
													int, int, int, int);
string getOrientString(REO_POS, REO_MODEL_TYPE);

bool ge(int, int);
bool le(int, int);
bool lt(int, int);

void extractBase(SentenceAlignment &);
void extract(SentenceAlignment &);
void addPhrase(SentenceAlignment &, int, int, int, int, string &);
bool isAligned (SentenceAlignment &, int, int);

bool allModelsOutputFlag = false;

bool wordModel = false;
REO_MODEL_TYPE wordType = REO_MSD;
bool phraseModel = false;
REO_MODEL_TYPE phraseType = REO_MSD;
bool hierModel = false;
REO_MODEL_TYPE hierType = REO_MSD;
//int modelMaxDistance;

bool pipeOut = false;

Bz2LineWriter* extractFile;
Bz2LineWriter* extractFileInv;
Bz2LineWriter* extractFileOrientation;

int maxPhraseLength;
bool orientationFlag = false;
bool onlyOutputSpanInfo = false;

int main(int argc, char* argv[]) {
  cerr	<< "PhraseExtract " << extract_version << endl << "Written by Philipp Koehn" << endl
				<< "Modified by Ventsislav Zhechev, Autodesk Development Sàrl" << endl
				<< "phrase extraction from an aligned parallel corpus" << endl
	;
	
  if (argc < 6) {
    cerr << "syntax: extract en de align extract max-length [orientation [ --model [wbe|phrase|hier]-[msd|mslr|mono]-max_distance ] | --OnlyOutputSpanInfo]\n";
    exit(1);
  }
	
  const string fileNameE = argv[1];
  const string fileNameF = argv[2];
  const string fileNameA = argv[3];
  const string fileNameExtract = argv[4];
  maxPhraseLength = atoi(argv[5]);
	
  for (int i=6; i<argc; ++i) {
    if (strcmp(argv[i], "--OnlyOutputSpanInfo") == 0)
      onlyOutputSpanInfo = true;
    else if (strcmp(argv[i], "orientation") == 0 || strcmp(argv[i], "--Orientation") == 0)
      orientationFlag = true;
		else if (strcmp(argv[i], "--pipeOut") == 0)
			pipeOut = true;
    else if (strcmp(argv[i], "--model") == 0) {
      if (i+1 >= argc) {
				cerr << "extract: syntax error, no model information provided to the option --model " << endl;
				exit(1);
      }
			
			
      const string modelParams = argv[++i];
      const string modelName = modelParams.substr(0, modelParams.find('-'));
      const string modelType = modelParams.substr(modelParams.find('-') + 1);
//			modelMaxDistance = modelParams.find('-', modelParams.find('-') + 1) ? atoi(modelParams.substr(modelParams.find_last_of('-') + 1).c_str()) : 6;
			
      if (modelName == "wbe"){
				wordModel = true;
				if (modelType == "msd")
					wordType = REO_MSD;
				else if (modelType == "mslr")
					wordType = REO_MSLR;
				else if (modelType == "mono" || modelType == "monotonicity")
					wordType = REO_MONO;
				else {
					cerr << "extract: syntax error, unknown reordering model type: " << modelType << endl;
					exit(1);
				}
      } else if (modelName == "phrase") {
				phraseModel = true;
				if (modelType == "msd")
					phraseType = REO_MSD;
				else if (modelType == "mslr")
					phraseType = REO_MSLR;
				else if (modelType == "mono" || modelType == "monotonicity")
					phraseType = REO_MONO;
				else {
					cerr << "extract: syntax error, unknown reordering model type: " << modelType << endl;
					exit(1);
				}
      } else if (modelName == "hier") {
				hierModel = true;
				if (modelType == "msd")
					hierType = REO_MSD;
				else if (modelType == "mslr")
					hierType = REO_MSLR;
				else if (modelType == "mono" || modelType == "monotonicity")
					hierType = REO_MONO;
				else {
					cerr << "extract: syntax error, unknown reordering model type: " << modelType << endl;
					exit(1);
				}
      } else {
				cerr << "extract: syntax error, unknown reordering model: " << modelName << endl;
				exit(1);
      }
			
      allModelsOutputFlag = true;
    } else {
      cerr << "extract: syntax error, unknown option '" << argv[i] << "'" << endl;
      exit(1);
    }
  }
	
  // default reordering model if no model selected
  // allows for the old syntax to be used
  if(orientationFlag && !allModelsOutputFlag) {
    wordModel = true;
    wordType = REO_MSD;
  }
	
  // open input files
	Bz2LineReader eFile(fileNameE);
	Bz2LineReader fFile(fileNameF);
	Bz2LineReader aFile(fileNameA);
	
  // open output files
	cerr << "Outputting to " << (pipeOut ? "pipes" : "bzip2-ed files") << "…" << endl;
	string extention = pipeOut ? ".pipe" : ".bz2";
  extractFile = new Bz2LineWriter(fileNameExtract + extention);
  extractFileInv = new Bz2LineWriter(fileNameExtract + ".inv" + extention);
  if (orientationFlag)
    extractFileOrientation = new Bz2LineWriter(fileNameExtract + ".o" + extention);
	
	for (int i = 0;;) {
		if ((++i)%500000 == 0) cerr << "[extract:" << i << "]" << flush;
    else if (i%10000 == 0) cerr << "." << flush;
		
		string englishString = eFile.readLine();
		if (englishString.empty()) {
//			cerr << "Finished extraction at line " << i << "!" << endl;
			break;
		}
		string foreignString = fFile.readLine();
		string alignmentString = aFile.readLine();

    SentenceAlignment sentence;

    //az: output src, tgt, and alingment line
    if (onlyOutputSpanInfo) {
      cout << "LOG: SRC: " << foreignString << endl;
      cout << "LOG: TGT: " << englishString << endl;
      cout << "LOG: ALT: " << alignmentString << endl;
      cout << "LOG: PHRASES_BEGIN:" << endl;
    }
		
    if (sentence.create(englishString, foreignString, alignmentString, i))
      extract(sentence);
    if (onlyOutputSpanInfo) cout << "LOG: PHRASES_END:" << endl; //az: mark end of phrases
  }
	
  eFile.close();
  fFile.close();
  aFile.close();
  //az: only close if we actually opened it
	if (!onlyOutputSpanInfo) {
		extractFile->close();
		extractFileInv->close();
		if (orientationFlag) extractFileOrientation->close();
	}
}

void extract(SentenceAlignment &sentence) {
  int countE = sentence.target.size();
  int countF = sentence.source.size();
	
  HPhraseVector inboundPhrases;
	
  HSentenceVertices inTopLeft;
  HSentenceVertices inTopRight;
  HSentenceVertices inBottomLeft;
  HSentenceVertices inBottomRight;
	
  HSentenceVertices outTopLeft;
  HSentenceVertices outTopRight;
  HSentenceVertices outBottomLeft;
  HSentenceVertices outBottomRight;
	
  HSentenceVertices::const_iterator it;
	
  bool relaxLimit = hierModel;
  bool buildExtraStructure = phraseModel || hierModel;
	
  // check alignments for target phrase startE...endE
  // loop over extracted phrases which are compatible with the word-alignments
  for(int startE=0; startE<countE; ++startE) {
    for(int endE=startE; (endE<countE && (relaxLimit || endE<startE+maxPhraseLength)); ++endE) {
			
      int minF = 9999;
      int maxF = -1;
      vector<int> usedF = sentence.alignedCountS;
      for(int ei=startE;ei<=endE;++ei)
				for(int i=0;i<sentence.alignedToT[ei].size();++i) {
					int fi = sentence.alignedToT[ei][i];
					if (fi<minF) { minF = fi; }
					if (fi>maxF) { maxF = fi; }
					--usedF[fi];
				}
			
      if (maxF >= 0 && // aligned to any source words at all
					(relaxLimit || maxF-minF < maxPhraseLength)) { // source phrase within limits
				
				// check if source words are aligned to out of bound target words
				bool out_of_bounds = false;
				for(int fi=minF;fi<=maxF && !out_of_bounds;++fi)
					out_of_bounds = usedF[fi] > 0;
				
				// cout << "doing if for ( " << minF << "-" << maxF << ", " << startE << "," << endE << ")\n";
				if (!out_of_bounds){
					// start point of source phrase may retreat over unaligned
					for(int startF=minF;
							(startF>=0 &&
							 (relaxLimit || startF>maxF-maxPhraseLength) && // within length limit
							 (startF==minF || sentence.alignedCountS[startF]==0)); // unaligned
							--startF)
						// end point of source phrase may advance over unaligned
						for(int endF=maxF;
								(endF<countF &&
								 (relaxLimit || endF<startF+maxPhraseLength) && // within length limit
								 (endF==maxF || sentence.alignedCountS[endF]==0)); // unaligned
								++endF){ // at this point we have extracted a phrase
							if(buildExtraStructure){ // phrase || hier
								if(endE-startE < maxPhraseLength && endF-startF < maxPhraseLength){ // within limit
									inboundPhrases.push_back(HPhrase(HPhraseVertex(startF,startE),
																									 HPhraseVertex(endF,endE)));
									insertPhraseVertices(inTopLeft, inTopRight, inBottomLeft, inBottomRight,
																			 startF, startE, endF, endE);
								} else
									insertPhraseVertices(outTopLeft, outTopRight, outBottomLeft, outBottomRight,
																			 startF, startE, endF, endE);
							} else {
								string orientationInfo = "";
								if(wordModel) {
									REO_POS wordPrevOrient, wordNextOrient;
									bool connectedLeftTopP  = isAligned( sentence, startF-1, startE-1 );
									bool connectedRightTopP = isAligned( sentence, endF+1,   startE-1 );
									bool connectedLeftTopN  = isAligned( sentence, endF+1, endE+1 );
									bool connectedRightTopN = isAligned( sentence, startF-1,   endE+1 );
									wordPrevOrient = getOrientWordModel(sentence, wordType, connectedLeftTopP, connectedRightTopP, startF, endF, startE, endE, countF, 0, 1, &ge, &lt);
									wordNextOrient = getOrientWordModel(sentence, wordType, connectedLeftTopN, connectedRightTopN, endF, startF, endE, startE, 0, countF, -1, &lt, &ge);
									orientationInfo += getOrientString(wordPrevOrient, wordType) + " " + getOrientString(wordNextOrient, wordType);
//									if(allModelsOutputFlag)
//										" | | ";
								}
								addPhrase(sentence, startE, endE, startF, endF, orientationInfo);
							}
						}
				}
      }
    }
  }
	
  if(buildExtraStructure){ // phrase || hier
    string orientationInfo = "";
    REO_POS wordPrevOrient, wordNextOrient, phrasePrevOrient, phraseNextOrient, hierPrevOrient, hierNextOrient;
		
    for(size_t i = 0; i < inboundPhrases.size(); ++i){
      int startF = inboundPhrases[i].first.first;
      int startE = inboundPhrases[i].first.second;
      int endF = inboundPhrases[i].second.first;
      int endE = inboundPhrases[i].second.second;
			
      bool connectedLeftTopP  = isAligned(sentence, startF-1, startE-1);
      bool connectedRightTopP = isAligned(sentence, endF+1,   startE-1);
      bool connectedLeftTopN  = isAligned(sentence, endF+1,   endE+1);
      bool connectedRightTopN = isAligned(sentence, startF-1, endE+1);
      
      if(wordModel){
				wordPrevOrient = getOrientWordModel(sentence, wordType,
																						connectedLeftTopP, connectedRightTopP,
																						startF, endF, startE, endE, countF, 0, 1,
																						&ge, &lt);
				wordNextOrient = getOrientWordModel(sentence, wordType,
																						connectedLeftTopN, connectedRightTopN,
																						endF, startF, endE, startE, 0, countF, -1,
																						&lt, &ge);
      }
			
      if (phraseModel) {
				phrasePrevOrient = getOrientPhraseModel(sentence, phraseType, 
																								connectedLeftTopP, connectedRightTopP,
																								startF, endF, startE, endE, countF-1, 0, 1, &ge, &lt, inBottomRight, inBottomLeft);
				phraseNextOrient = getOrientPhraseModel(sentence, phraseType,
																								connectedLeftTopN, connectedRightTopN,
																								endF, startF, endE, startE, 0, countF-1, -1, &lt, &ge, inBottomLeft, inBottomRight);
      } else
				phrasePrevOrient = phraseNextOrient = UNKNOWN;
			
      if(hierModel){
				hierPrevOrient = getOrientHierModel(sentence, hierType, 
																						connectedLeftTopP, connectedRightTopP,
																						startF, endF, startE, endE, countF-1, 0, 1, &ge, &lt, inBottomRight, inBottomLeft, outBottomRight, outBottomLeft, phrasePrevOrient);
				hierNextOrient = getOrientHierModel(sentence, hierType,
																						connectedLeftTopN, connectedRightTopN,
																						endF, startF, endE, startE, 0, countF-1, -1, &lt, &ge, inBottomLeft, inBottomRight, outBottomLeft, outBottomRight, phraseNextOrient);
      }
			
      orientationInfo = ((wordModel)? getOrientString(wordPrevOrient, wordType) + " " + getOrientString(wordNextOrient, wordType) : "") + " | " +
			((phraseModel)? getOrientString(phrasePrevOrient, phraseType) + " " + getOrientString(phraseNextOrient, phraseType) : "") + " | " +
			((hierModel)? getOrientString(hierPrevOrient, hierType) + " " + getOrientString(hierNextOrient, hierType) : "");
			
      addPhrase(sentence, startE, endE, startF, endF, orientationInfo);
    }
  }
}

REO_POS getOrientWordModel(SentenceAlignment & sentence, REO_MODEL_TYPE modelType,
													 bool connectedLeftTop, bool connectedRightTop,
													 int startF, int endF, int startE, int endE, int countF, int zero, int unit,
													 bool (*ge)(int, int), bool (*lt)(int, int) ){
	
	if(connectedLeftTop && !connectedRightTop)
    return LEFT;
  if(modelType == REO_MONO)
    return UNKNOWN;
  if (!connectedLeftTop &&  connectedRightTop)
    return RIGHT;
  if(modelType == REO_MSD)
    return UNKNOWN;
  for(int indexF=startF-2*unit; (*ge)(indexF, zero) && !connectedLeftTop; indexF=indexF-unit)
    connectedLeftTop = isAligned(sentence, indexF, startE-unit);
  for(int indexF=endF+2*unit; (*lt)(indexF,countF) && !connectedRightTop; indexF=indexF+unit)
    connectedRightTop = isAligned(sentence, indexF, startE-unit);
  if(connectedLeftTop && !connectedRightTop)
    return DRIGHT;
  else if(!connectedLeftTop && connectedRightTop)
    return DLEFT;
  return UNKNOWN;
}

// to be called with countF-1 instead of countF
REO_POS getOrientPhraseModel (SentenceAlignment & sentence, REO_MODEL_TYPE modelType,
															bool connectedLeftTop, bool connectedRightTop,
															int startF, int endF, int startE, int endE, int countF, int zero, int unit,
															bool (*ge)(int, int), bool (*lt)(int, int),
															const HSentenceVertices & inBottomRight, const HSentenceVertices & inBottomLeft){
	
  HSentenceVertices::const_iterator it;
	
  if((connectedLeftTop && !connectedRightTop) ||
     ((it = inBottomRight.find(startE - unit)) != inBottomRight.end() &&
      it->second.find(startF-unit) != it->second.end()))
    return LEFT;
  if(modelType == REO_MONO)
    return UNKNOWN;
  if((!connectedLeftTop &&  connectedRightTop) ||
     ((it = inBottomLeft.find(startE - unit)) != inBottomLeft.end() && it->second.find(endF + unit) != it->second.end()))
    return RIGHT;
  if(modelType == REO_MSD)
    return UNKNOWN;
  connectedLeftTop = false;
  for(int indexF=startF-2*unit; (*ge)(indexF, zero) && !connectedLeftTop; indexF=indexF-unit)
    if((connectedLeftTop = (it = inBottomRight.find(startE - unit)) != inBottomRight.end()) && it->second.find(indexF) != it->second.end())
      return DRIGHT;
  connectedRightTop = false;
  for(int indexF=endF+2*unit; (*lt)(indexF, countF) && !connectedRightTop; indexF=indexF+unit)
    if((connectedRightTop = (it = inBottomLeft.find(startE - unit)) != inBottomRight.end()) && it->second.find(indexF) != it->second.end())
      return DLEFT;
  return UNKNOWN;
}

// to be called with countF-1 instead of countF
REO_POS getOrientHierModel (SentenceAlignment & sentence, REO_MODEL_TYPE modelType,
														bool connectedLeftTop, bool connectedRightTop,
														int startF, int endF, int startE, int endE, int countF, int zero, int unit,
														bool (*ge)(int, int), bool (*lt)(int, int),
														const HSentenceVertices & inBottomRight, const HSentenceVertices & inBottomLeft,
														const HSentenceVertices & outBottomRight, const HSentenceVertices & outBottomLeft,
														REO_POS phraseOrient){
	
  HSentenceVertices::const_iterator it;
  
  if(phraseOrient == LEFT ||
     (connectedLeftTop && !connectedRightTop) ||
     ((it = inBottomRight.find(startE - unit)) != inBottomRight.end() &&
      it->second.find(startF-unit) != it->second.end()) || 
     ((it = outBottomRight.find(startE - unit)) != outBottomRight.end() &&
      it->second.find(startF-unit) != it->second.end()))
    return LEFT;
  if(modelType == REO_MONO)
    return UNKNOWN;  
  if(phraseOrient == RIGHT || 
     (!connectedLeftTop &&  connectedRightTop) ||
     ((it = inBottomLeft.find(startE - unit)) != inBottomLeft.end() && 
      it->second.find(endF + unit) != it->second.end()) ||
     ((it = outBottomLeft.find(startE - unit)) != outBottomLeft.end() && 
      it->second.find(endF + unit) != it->second.end()))
    return RIGHT;
  if(modelType == REO_MSD)
    return UNKNOWN;
  if(phraseOrient != UNKNOWN)
    return phraseOrient;
  connectedLeftTop = false;
  for(int indexF=startF-2*unit; (*ge)(indexF, zero) && !connectedLeftTop; indexF=indexF-unit) {
    if((connectedLeftTop = (it = inBottomRight.find(startE - unit)) != inBottomRight.end() &&
				it->second.find(indexF) != it->second.end()) ||
       (connectedLeftTop = (it = outBottomRight.find(startE - unit)) != outBottomRight.end() &&
				it->second.find(indexF) != it->second.end()))
      return DRIGHT;
  }
  connectedRightTop = false;
  for(int indexF=endF+2*unit; (*lt)(indexF, countF) && !connectedRightTop; indexF=indexF+unit) {
    if((connectedRightTop = (it = inBottomLeft.find(startE - unit)) != inBottomRight.end() &&
				it->second.find(indexF) != it->second.end()) ||
       (connectedRightTop = (it = outBottomLeft.find(startE - unit)) != outBottomRight.end() &&
				it->second.find(indexF) != it->second.end()))
      return DLEFT;
  }
  return UNKNOWN;
}

bool isAligned (SentenceAlignment &sentence, int fi, int ei){
  if (ei == -1 && fi == -1)
    return true;
  if (ei <= -1 || fi <= -1)
    return false;
  if (ei == sentence.target.size() && fi == sentence.source.size())
    return true;
  if (ei >= sentence.target.size() || fi >= sentence.source.size())
    return false;
  for(int i=0;i<sentence.alignedToT[ei].size(); ++i)
    if (sentence.alignedToT[ei][i] == fi)
      return true;
  return false;
}

inline bool ge(int first, int second){
  return first >= second;
}

inline bool le(int first, int second){
  return first <= second;
}

inline bool lt(int first, int second){
  return first < second;
}

void insertVertex( HSentenceVertices & corners, int x, int y ){
  set<int> tmp;
  tmp.insert(x);
  pair<HSentenceVertices::iterator, bool> ret = corners.insert(make_pair(y, tmp));
  if (ret.second == false)
    ret.first->second.insert(x);
}

void insertPhraseVertices(
													HSentenceVertices & topLeft,
													HSentenceVertices & topRight,
													HSentenceVertices & bottomLeft,
													HSentenceVertices & bottomRight,
													int startF, int startE, int endF, int endE) {
	
  insertVertex(topLeft, startF, startE);
  insertVertex(topRight, endF, startE);
  insertVertex(bottomLeft, startF, endE);
  insertVertex(bottomRight, endF, endE);
}

string getOrientString(REO_POS orient, REO_MODEL_TYPE modelType){
  switch(orient) {
		case LEFT: return "mono"; break;
		case RIGHT: return "swap"; break;
		case DRIGHT: return "dright"; break;
		case DLEFT: return "dleft"; break;
		case UNKNOWN:
			switch(modelType){
				case REO_MONO: return "nomono"; break;
				case REO_MSD: return "other"; break;
				case REO_MSLR: return "dright"; break;
			}
			break;
  }
	return "";
}

void addPhrase( SentenceAlignment &sentence, int startE, int endE, int startF, int endF , string &orientationInfo) {
	
  if (onlyOutputSpanInfo) {
    cout << startF << " " << endF << " " << startE << " " << endE << endl;
    return;
  }
	
	// source
  for(int fi=startF;fi<=endF;++fi) {
    extractFile->writeLine(sentence.source[fi] + " ");
    if (orientationFlag) extractFileOrientation->writeLine(sentence.source[fi] + " ");
  }
  extractFile->writeLine("||| ");
  if (orientationFlag) extractFileOrientation->writeLine("||| ");
	
  // target
  for(int ei=startE;ei<=endE;++ei) {
    extractFile->writeLine(sentence.target[ei] + " ");
    extractFileInv->writeLine(sentence.target[ei] + " ");
    if (orientationFlag) extractFileOrientation->writeLine(sentence.target[ei] + " ");
  }
  extractFile->writeLine("|||");
  extractFileInv->writeLine("||| ");
  if (orientationFlag) extractFileOrientation->writeLine("||| ");
	
  // source (for inverse)
  for(int fi=startF;fi<=endF;++fi)
    extractFileInv->writeLine(sentence.source[fi] + " ");
  extractFileInv->writeLine("|||");
	
  // alignment
	stringstream extractFileSS, extractFileInvSS;
  for(int ei=startE;ei<=endE;++ei)
    for(size_t i=0;i<sentence.alignedToT[ei].size();++i) {
      int fi = sentence.alignedToT[ei][i];
      extractFileSS << " " << fi-startF << "-" << ei-startE;
      extractFileInvSS << " " << ei-startE << "-" << fi-startF;
    }
	
	extractFile->writeLine(extractFileSS.str() + "\n");
  extractFileInv->writeLine(extractFileInvSS.str() + "\n");
  if (orientationFlag)
    extractFileOrientation->writeLine(orientationInfo + "\n");
}

// if proper conditioning, we need the number of times a source phrase occured
void extractBase( SentenceAlignment &sentence ) {
  int countF = sentence.source.size();
  for(int startF=0;startF<countF;++startF) {
    for(int endF=startF;
        (endF<countF && endF<startF+maxPhraseLength);
        endF++) {
      for(int fi=startF;fi<=endF;++fi) {
				extractFile->writeLine(sentence.source[fi] + " ");
      }
      extractFile->writeLine("|||\n");
    }
  }
	
  int countE = sentence.target.size();
  for(int startE=0;startE<countE;++startE) {
    for(int endE=startE;
        (endE<countE && endE<startE+maxPhraseLength);
        endE++) {
      for(int ei=startE;ei<=endE;++ei) {
				extractFileInv->writeLine(sentence.target[ei] + " ");
      }
      extractFileInv->writeLine("|||\n");
    }
  }
}
