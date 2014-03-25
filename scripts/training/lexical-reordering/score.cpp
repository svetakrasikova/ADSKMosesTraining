/*
 * score_reordering.cpp
 *
 *      Created by: © Sara Stymne - Linköping University
 *      Machine Translation Marathon 2010, Dublin
 *			© 2011 Autodesk Development Sàrl
 *			Last modified by Ventsislav Zhechev on 18 Mar 2011
 */

#include <string>
#include <vector>
#include <map>
#include <iostream>

#include "reordering_classes.h"

#include "Bz2LineReader.h"

using namespace std;
using namespace bg_zhechev_ventsislav;


void split_line(const string& line, string& foreign, string& english, string& wbe, string& phrase, string& hier);
void get_orientations(const string& pair, string& previous, string& next);


int main(int argc, char* argv[]) {
	
  cerr	<< "Lexical Reordering Scorer v1.5 written by Sara Stymne" << endl
				<< "Modified by Ventsislav Zhechev, Autodesk Development Sàrl" << endl
				<< "scores lexical reordering models of several types (hierarchical, phrase-based and word-based extraction)" << endl
	;
	
  if (argc < 3) {
    cerr << "syntax: score_reordering extractFile smoothingValue filepath (--model \"type max-orientation (specification-strings)\" )+\n";
    exit(1);
  }
	
  string extractFileName = argv[1];
  double smoothingValue = strtod(argv[2], NULL);
  string filepath = argv[3];
	
  bool smoothWithCounts = false;
  map<string, ModelScore*> modelScores;
  vector<Model*> models;
  bool hier = false;
  bool phrase = false;
  bool wbe = false;
	
  string e,f,w,p,h;
  string prev, next;
	
  for (size_t i = 4; i < argc; ++i) {
    if (strcmp(argv[i],"--SmoothWithCounts") == 0) {
      smoothWithCounts = true;
    } else if (strcmp(argv[i],"--model") == 0) {
      if (i+1 >= argc){
				cerr << "score: syntax error, no model information provided to the option" << argv[i] << endl;
				exit(1);
      }
			
			//The next command-line argument is a ""-quoted string and contains space-separated elements that will be parsed below
      istringstream is(argv[++i]);
      string m,t;
      is >> m >> t;
      modelScores[m] = ModelScore::createModelScore(t);
			hier = m == "hier";
			phrase = !hier && m == "phrase";
			wbe = !hier && !phrase && m == "wbe";
			
      if (!hier && !phrase && !wbe) {
				cerr << "WARNING: No models specified for lexical reordering. No lexical reordering table will be trained.\n";
				exit(1);
      }
			
      string config;
      //Store all models
      while (is >> config)
				models.push_back(Model::createModel(modelScores[m], config, filepath));
    } else {
      cerr << "Illegal option given to lexical reordering model score" << endl;
      exit(1);
    }
  }
	
	if (filepath == "-" && models.size() != 1) {
		cerr << "It is not possible to score more than one model when writing to standart output!" << endl;
		exit(2);
	}
	
  ////////////////////////////////////
  //calculate smoothing
  if (!smoothWithCounts)
    //constant smoothing
    for (size_t i=0; i<models.size(); models[i++]->createConstSmoothing(smoothingValue));
	else {
		if (extractFileName == "-") {
			cerr << "The ‘smoothWithCounts’ option may not be used with piped input!" << endl;
			exit(9);
		}
		Bz2LineReader extractFile(extractFileName);
		unsigned i = 0;
		for (string line = extractFile.readLine(); !line.empty(); line = extractFile.readLine()) {
			if (line.empty()) break;
			if ((++i)%10000000 == 0) cerr << "[l. score:" << i << "]" << flush;
			else if (i%100000 == 0) cerr << "," << flush;
      split_line(line,e,f,w,p,h);
      if (hier) {
				get_orientations(h, prev, next);
				modelScores["hier"]->add_example(prev,next);
      } 
      if (phrase) {
				get_orientations(p, prev, next);
				modelScores["phrase"]->add_example(prev,next);
      } 
      if (wbe) {
				get_orientations(w, prev, next);
				modelScores["wbe"]->add_example(prev,next);
      }
    }
		
    // calculate smoothing for each model
    for (size_t i=0; i<models.size(); models[i++]->createSmoothing(smoothingValue));

		extractFile.close();
  }
	
  ////////////////////////////////////
  //calculate scores for reordering table
  string f_current,e_current;
	bool first = true;
	Bz2LineReader extractFile(extractFileName);
	unsigned i = 0;
	for (string line = extractFile.readLine(); !line.empty(); line = extractFile.readLine()) {
		if (line.empty()) break;
		if ((++i)%10000000 == 0) cerr << "[l. score:" << i << "]" << flush;
		else if (i%100000 == 0) cerr << "." << flush;
    split_line(line,f,e,w,p,h);
		
    if (first) {
      f_current = f;
      e_current = e;
      first = false;
    } else if (f != f_current || e != e_current) {
      //fe - score
      for (int i=0; i<models.size(); models[i++]->score_fe(f_current, e_current));
      //reset
      for(map<string,ModelScore*>::const_iterator it = modelScores.begin(); it != modelScores.end(); (it++)->second->reset_fe());
      
      if (f != f_current) {
				//f - score
				for (int i=0; i<models.size(); models[i++]->score_f(f_current));
				//reset
				for(map<string,ModelScore*>::const_iterator it = modelScores.begin(); it != modelScores.end(); (it++)->second->reset_f());
      }

      f_current = f;
      e_current = e;
    }
		
    // uppdate counts
    if (hier) {
      get_orientations(h, prev, next);
      modelScores["hier"]->add_example(prev,next);
    } 
    if (phrase) {
      get_orientations(p, prev, next);
      modelScores["phrase"]->add_example(prev,next);
    } 
    if (wbe) {
      get_orientations(w, prev, next);
      modelScores["wbe"]->add_example(prev,next);
    }
  }
  //Score the last phrases
  for (size_t i=0; i<models.size(); models[i++]->score_fe(f,e));
  for (size_t i=0; i<models.size(); models[i++]->score_f(f));

	//Release the memory
	for (size_t i=0; i<models.size(); delete models[i++]);
	
  return 0;
}



void split_line(const string& line, string& foreign, string& english, string& wbe, string& phrase, string& hier) {
	
  string::size_type begin = 0;
	string::size_type end = line.find(" ||| ");
  foreign = line.substr(begin, end - begin);
	
  begin = end+5;
  end = line.find(" ||| ", begin);
  english = line.substr(begin, end - begin);
	
  begin = end+5;
  end = line.find(" | ", begin);
  wbe = line.substr(begin, end - begin);
	
  begin = end+3;
  end = line.find(" | ", begin);
  phrase = line.substr(begin, end - begin);
  
  begin = end+3;
  hier = line.substr(begin, line.size() - begin);
}

void get_orientations(const string& pair, string& previous, string& next) {
  istringstream is(pair);
  is >> previous >> next;
}
