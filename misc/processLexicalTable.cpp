/*
 *
 *      Creater unknown
 *			© 2011 Autodesk Development Sˆrl
 *			Last modified by Ventsislav Zhechev on 13 Mar 2011
 *
 */

#include <iostream>
#include <string>

//#include "Timer.h"
#include "InputFileStream.h"
#include "LexicalReorderingTable.h"

using namespace std;
using namespace Moses;

//Timer timer;

void printHelp(){
  cerr << "Usage:" << endl <<
	"options: " << endl <<
	"\t-in  string -- input table file name" << endl <<
	"\t-out string -- prefix of binary table files" << endl <<
	"If -in is not specified, reads from stdin" << endl <<
	endl; 
}

int main(int argc, char** argv){
  cerr << "processLexicalTable v0.1 by Konrad Rawlik\n";
  string inFilePath;
  string outFilePath("out");
  if(argc <= 1){
		printHelp();
		exit(1);
  }
	for (int i = 1; i < argc; ++i)
		if ((string)argv[i] == "-in" && i+1 < argc)
      inFilePath = argv[++i];
		else if ((string)argv[i] == "-out" && i+1 < argc)
      outFilePath = argv[++i];
		else {
      //somethingÕs wrong... print help
			printHelp();
      exit(1);
    }
  
  if (inFilePath.empty()) {
		cerr << "processing stdin to " << outFilePath << ".*" << endl;
		if (!LexicalReorderingTableTree::Create(cin, outFilePath)) exit(2);
  } else {
		cerr << "processing " << inFilePath << " to " << outFilePath << ".*" << endl;
    InputFileStream file(inFilePath);
    if (!LexicalReorderingTableTree::Create(file, outFilePath)) exit(3);
  }
	
	return 0;
}
