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

#pragma once
#ifndef SENTENCE_ALIGNMENT_H_INCLUDED_
#define SENTENCE_ALIGNMENT_H_INCLUDED_

#include <string>
#include <vector>

using namespace std;

struct SentenceAlignment {
	vector<string> target;
	vector<string> source;
	vector<int> alignedCountS;
	vector<vector<int> > alignedToT;
	
	virtual void processTargetSentence(const string&);
	
	virtual void processSourceSentence(const string&);
	
	bool create(const string& targetString, const string& sourceString, const string& alignmentString, int sentenceID);
};

#endif
