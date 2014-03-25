/*
 * reordering_classes.h
 * Utility classes for lexical reordering table scoring
 *
 *      Created by: © Sara Stymne - Linköping University
 *      Machine Translation Marathon 2010, Dublin
 *			© 2011 Autodesk Development Sàrl
 *			Last modified by Ventsislav Zhechev on 18 Mar 2011
 */

#pragma once

#include <vector>
#include <numeric>
#include <sstream>
#include <string>

#include "Bz2LineWriter.h"

using namespace std;
using namespace bg_zhechev_ventsislav;

enum ORIENTATION {MONO, SWAP, DRIGHT, DLEFT, OTHER, NOMONO};


//Keeps the counts for the different reordering types 
//(Instantiated in 1-3 instances, one for each type of model (hier, phrase, wbe))
class ModelScore {
  vector<double> count_fe_prev;
  vector<double> count_fe_next;
  vector<double> count_f_prev;
  vector<double> count_f_next;
	
protected:
  virtual ORIENTATION getType(const string& s); 
	
public:
  ModelScore();
  void add_example(const string& previous, string& next);
  void reset_fe();
  void reset_f();
  inline const vector<double>& get_scores_fe_prev() const { return count_fe_prev; }
  inline const vector<double>& get_scores_fe_next() const { return count_fe_next; }
  inline const vector<double>& get_scores_f_prev() const { return count_f_prev; }
  inline const vector<double>& get_scores_f_next() const { return count_f_next; }
	
  static ModelScore* createModelScore(const string& modeltype);
};

class ModelScoreMSLR : public ModelScore {
protected:
  virtual ORIENTATION getType(const string& s);
};

class ModelScoreLR : public ModelScore {
protected:
  virtual ORIENTATION getType(const string& s);
};

class ModelScoreMSD : public ModelScore {
protected:
  virtual ORIENTATION getType(const string& s);
};

class ModelScoreMonotonicity : public ModelScore {
protected:
  virtual ORIENTATION getType(const string& s);
};

//Class for calculating total counts, and to calculate smoothing
struct Scorer {
  virtual void score(const vector<double>&, vector<double>&) const = 0;
  virtual void createSmoothing(const vector<double>&, double, vector<double>&) const = 0;
  virtual void createConstSmoothing(double, vector<double>&) const = 0;
};

struct ScorerMSLR : public Scorer {
  virtual void score(const vector<double>&, vector<double>&) const;
  virtual void createSmoothing(const vector<double>&, double, vector<double>&) const;
  virtual void createConstSmoothing(double, vector<double>&) const;
};

struct ScorerMSD : public Scorer {
  virtual void score(const vector<double>&, vector<double>&) const;
  virtual void createSmoothing(const vector<double>&, double, vector<double>&) const;
  virtual void createConstSmoothing(double, vector<double>&) const;
};

struct ScorerMonotonicity : public Scorer {
  virtual void score(const vector<double>&, vector<double>&) const;
  virtual void createSmoothing(const vector<double>&, double, vector<double>&) const;
  virtual void createConstSmoothing(double, vector<double>&) const;
};

struct ScorerLR : public Scorer {
  virtual void score(const vector<double>&, vector<double>&) const;
  virtual void createSmoothing(const vector<double>&, double, vector<double>&) const;
  virtual void createConstSmoothing(double, vector<double>&) const;
};


//Class for representing each model 
//Contains a modelscore and scorer (which can be of different model types (mslr, msd...)), and file handling. 
//This class also keeps track of bidirectionality, and which language to condition on
class Model {
  ModelScore* modelscore;
  Scorer* scorer;
	
	Bz2LineWriter* outputFile;
	
  bool fe;
  bool previous;
  bool next;
	
  vector<double> smoothing_prev;
  vector<double> smoothing_next;

	//Hide the default constructor
	Model() {}
public:
  Model(ModelScore* ms, Scorer* sc, const string& dir, const string& lang, const string& fn);
  ~Model();
  static Model* createModel(ModelScore*, string, const string&);
  void createSmoothing(double w);
  void createConstSmoothing(double w);
  void score_fe(const string& f, const string& e);
  void score_f(const string& f);
};

