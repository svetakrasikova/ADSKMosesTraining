/***********************************************************************
  Moses - factored phrase-based language decoder
  © 2009 University of Edinburgh
  © 2011 Autodesk Development Sàrl

  Last modified by Ventsislav Zhechev on 15 Mar 2011
  Changes:
  1) Switched to reading and writing Bzip2-compressed data to reduce I/O operations
  2) Input and output can also automatically be done without compression to facilitate the use of named pipes

 
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

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <cassert>

#include "Bz2LineReader.h"
#include "Bz2LineWriter.h"

using namespace std;
using namespace bg_zhechev_ventsislav;

bool hierarchicalFlag = false;
bool logProbFlag = false;

vector<string> splitLine(const string&);

int main(int argc, char* argv[]) {
  cerr	<< "Consolidate v2.1 written by Philipp Koehn" << endl
				<< "Modified by Ventsislav Zhechev, Autodesk Development Sàrl" << endl
				<< "consolidating direct and indirect rule tables" << endl
	;

  if (argc < 4) {
    cerr << "syntax: consolidate phrase-table.direct phrase-table.indirect phrase-table.consolidated [--Hierarchical]\n";
    exit(1);
  }
  const string fileNameDirect = argv[1];
  const string fileNameIndirect = argv[2];
  const string fileNameConsolidated = argv[3];

  for(size_t i=4;i<argc;++i) {
    if (strcmp(argv[i],"--Hierarchical") == 0) {
      hierarchicalFlag = true;
      cerr << "processing hierarchical rules\n";
    } else if (strcmp(argv[i],"--LogProb") == 0) {
      logProbFlag = true;
      cerr << "using log-probabilities\n";
    } else {
      cerr << "ERROR: unknown option " << argv[i] << endl;
      exit(1);
    }
  }

  // open input files
	Bz2LineReader fileDirect(fileNameDirect);
	Bz2LineReader fileIndirect(fileNameIndirect);

  // open output file: consolidated phrase table
	Bz2LineWriter fileConsolidated(fileNameConsolidated);

  // loop through all extracted phrase translations
	stringstream streamConsolidated;
	for (unsigned i = 1; ; ++i) {
		string directLine = fileDirect.readLine();
		if (directLine.empty()) break;
		string indirectLine = fileIndirect.readLine();
		if (indirectLine.empty()) break;
		
		if (i % 10000000 == 0) cerr << "[consolidate:" << i << "]" << flush;
    if (i % 100000 == 0) cerr << "." << flush;

    vector<string> itemDirect = splitLine(directLine);
		vector<string> itemIndirect = splitLine(indirectLine);

    // direct: target source alignment probabilities
    // indirect: source target probabilities

    // consistency checks
		assert(itemDirect.size() == (hierarchicalFlag ? 5 : 4));
		assert(itemIndirect.size() == 4);
		assert(itemDirect[0] == itemIndirect[0]);
		assert(itemDirect[1] == itemIndirect[1]);
		
    // output hierarchical phrase pair (with separated labels)
    streamConsolidated << itemDirect[0] << " ||| " << itemDirect[1] << " ||| ";

    // output alignment and probabilities
    if (hierarchicalFlag)
      streamConsolidated << itemDirect[2]   << " ||| " // alignment
        << itemIndirect[2]      // prob indirect
        << " " << itemDirect[3]; // prob direct
    else
      streamConsolidated << itemIndirect[2]      // prob indirect
        << " " << itemDirect[2]; // prob direct
    streamConsolidated << " " << (logProbFlag ? 1 : 2.718); // phrase count feature

    // counts
    if (itemIndirect.size() == 4 && itemDirect.size() == 5)
      streamConsolidated << " ||| " << itemIndirect[3] << " " // indirect
        << itemDirect[4]; // direct

    streamConsolidated << endl;
		
		fileConsolidated.writeLine(streamConsolidated.str());
		streamConsolidated.str("");
  }
	
}

vector<string> splitLine(const string& ln) {
	char line[ln.length() + 1];
	strcpy(line, ln.c_str());
  vector<string> items;
	size_t start = 0, i = 0;
  for(; line[i] != '\0'; i++)
    if (line[i] == ' ' &&
        line[i+1] == '|' &&
        line[i+2] == '|' &&
        line[i+3] == '|' &&
        line[i+4] == ' ') {
      if (start > i)
				start = i; // empty item
      items.push_back( string( line+start, i-start ) );
      start = i+5;
      i += 3;
    }
	
	items.push_back( string( line+start, i-start ) );

  return items;
}
