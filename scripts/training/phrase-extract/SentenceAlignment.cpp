/***********************************************************************
  Moses - factored phrase-based language decoder
  © 2010 University of Edinburgh
  © 2011 Autodesk Development Sàrl

  Last modified by Ventsislav Zhechev on 18 Mar 2011


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

#include "SentenceAlignment.h"

#include <map>
#include <set>

#include "tables-core.h"

void SentenceAlignment::processTargetSentence(const string& targetString) {
    target = tokenize(targetString.c_str());
}

void SentenceAlignment::processSourceSentence(const string& sourceString) {
    source = tokenize(sourceString.c_str());
}

bool SentenceAlignment::create(const string& targetString, const string& sourceString, const string& alignmentString, int sentenceID) {

    // process sentence strings and store in target and source members.
    processTargetSentence(targetString);
    processSourceSentence(sourceString);

  // check if sentences are empty
  if (target.size() == 0 || source.size() == 0) {
    cerr << "no target (" << target.size() << ") or source (" << source.size() << ") words in sentence " << sentenceID << endl;
    cerr << "T: " << targetString << endl << "S: " << sourceString << endl;
    return false;
  }
	
  // prepare data structures for alignments
  for(int i=0; i<source.size(); i++) {
    alignedCountS.push_back( 0 );
  }
  for(int i=0; i<target.size(); i++) {
    vector< int > dummy;
    alignedToT.push_back( dummy );
  }
	
  // reading in alignments
  vector<string> alignmentSequence = tokenize(alignmentString.c_str());
  for (size_t i=0; i<alignmentSequence.size(); ++i) {
    int s,t;
    // cout << "scaning " << alignmentSequence[i].c_str() << endl;
    if (! sscanf(alignmentSequence[i].c_str(), "%d-%d", &s, &t)) {
      cerr << "WARNING: " << alignmentSequence[i] << " is a bad alignment point in sentence " << sentenceID << endl; 
      cerr << "T: " << targetString << endl << "S: " << sourceString << endl;
      return false;
    }
		// cout << "alignmentSequence[i] " << alignmentSequence[i] << " is " << s << ", " << t << endl;
    if (t >= target.size() || s >= source.size()) { 
      cerr << "WARNING: sentence " << sentenceID << " has alignment point (" << s << ", " << t << ") out of bounds (" << source.size() << ", " << target.size() << ")\n";
      cerr << "T: " << targetString << endl << "S: " << sourceString << endl;
      return false;
    }
    alignedToT[t].push_back( s );
    alignedCountS[s]++;
  }
  return true;
}
