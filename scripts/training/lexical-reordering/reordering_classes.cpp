/*
 *
 *      Created by: © Sara Stymne - Linköping University
 *      Machine Translation Marathon 2010, Dublin
 *			© 2011 Autodesk Development Sàrl
 *			Last modified by Ventsislav Zhechev on 18 Mar 2011
 *
 */


#include "reordering_classes.h"


ModelScore::ModelScore() {
  for(int i=MONO; i<=NOMONO; ++i) {
    count_fe_prev.push_back(0);
    count_fe_next.push_back(0);
    count_f_prev.push_back(0);
    count_f_next.push_back(0);
  }
}

ModelScore* ModelScore::createModelScore(const string& modeltype) {
  if (modeltype == "mslr")
    return new ModelScoreMSLR();
  else if (modeltype == "msd")
    return new ModelScoreMSD();
  else if (modeltype == "monotonicity")
    return new ModelScoreMonotonicity();
  else if (modeltype == "leftright")
    return new ModelScoreLR();
  else {
    cerr << "Illegal model type given for lexical reordering model scoring: " << modeltype << ". The allowed types are: mslr, msd, monotonicity, leftright" << endl;
    exit(1);
  }
}

void ModelScore::reset_fe() {
  for(int i=MONO; i<=NOMONO; ++i) {
    count_fe_prev[i] = 0;
    count_fe_next[i] = 0;
  }
}

void ModelScore::reset_f() {
  for(int i=MONO; i<=NOMONO; ++i) {
    count_f_prev[i] = 0;
    count_f_next[i] = 0;
  }
}

void ModelScore::add_example(const string& previous, string& next) { 
  count_fe_prev[getType(previous)]++;
  count_f_prev[getType(previous)]++;
  count_fe_next[getType(next)]++;
  count_f_next[getType(next)]++;
}


ORIENTATION ModelScore::getType(const string& type) {
  if (type == "mono")
    return MONO;
	else if (type == "swap")
    return SWAP;
  else if (type == "dright")
    return DRIGHT;
  else if (type == "dleft")
    return DLEFT;
  else if (type == "other")
    return OTHER;
  else if (type == "nomono")
    return NOMONO;
  else {
    cerr << "Illegal reordering type used: " << type << endl;
    exit(1);
  }
}


ORIENTATION ModelScoreMSLR::getType(const string& type) {
  if (type == "mono")
    return MONO;
  else if (type == "swap")
    return SWAP;
  else if (type == "dright")
    return DRIGHT;
  else if (type == "dleft")
    return DLEFT;
  else if (type == "other" || type == "nomono") {
    cerr << "Illegal reordering type used: " << type << " for model type mslr. You have to re-run step 5 in order to train such a model." <<  endl;
    exit(1);   
  } else {
    cerr << "Illegal reordering type used: " << type << endl;
    exit(1);
  }
}


ORIENTATION ModelScoreLR::getType(const string& type) {
  if (type == "mono" || type == "dright")
    return DRIGHT;
	else if (type == "swap" || type == "dleft")
		return DLEFT;
  else if (type == "other" || type == "nomono") {
    cerr << "Illegal reordering type used: " << type << " for model type LeftRight. You have to re-run step 5 in order to train such a model." <<  endl;
    exit(1);
  } else {
    cerr << "Illegal reordering type used: " << type << endl;
    exit(1);
  }
}


ORIENTATION ModelScoreMSD::getType(const string& type) {
  if (type == "mono")
    return MONO;
	else if (type == "swap")
    return SWAP;
	else if (type == "dleft" || type == "dright" || type == "other")
    return OTHER;
	else if (type == "nomono") {
    cerr << "Illegal reordering type used: " << type << " for model type msd. You have to re-run step 5 in order to train such a model." <<  endl;
    exit(1);
  } else {
    cerr << "Illegal reordering type used: " << type << endl;
    exit(1);
  }
}

ORIENTATION ModelScoreMonotonicity::getType(const string& type) {
  if (type == "mono")
    return MONO;
	else if (type == "swap" || type == "dleft" || type == "dright" || type == "other" || type == "nomono")
    return NOMONO;
	else {
    cerr << "Illegal reordering type used: " << type << endl;
    exit(1);
  }
}


  
void ScorerMSLR::score(const vector<double>& all_scores, vector<double>& scores) const {
  scores.push_back(all_scores[MONO]);
  scores.push_back(all_scores[SWAP]);
  scores.push_back(all_scores[DLEFT]);
  scores.push_back(all_scores[DRIGHT]);
}
  
void ScorerMSD::score(const vector<double>& all_scores, vector<double>& scores) const {
  scores.push_back(all_scores[MONO]);
  scores.push_back(all_scores[SWAP]);
  scores.push_back(all_scores[DRIGHT]+all_scores[DLEFT]+all_scores[OTHER]);
}

void ScorerMonotonicity::score(const vector<double>& all_scores, vector<double>& scores) const {
  scores.push_back(all_scores[MONO]);
  scores.push_back(all_scores[SWAP]+all_scores[DRIGHT]+all_scores[DLEFT]+all_scores[OTHER]+all_scores[NOMONO]);
}

  
void ScorerLR::score(const vector<double>& all_scores, vector<double>& scores) const {
  scores.push_back(all_scores[MONO]+all_scores[DRIGHT]);
  scores.push_back(all_scores[SWAP]+all_scores[DLEFT]);
}


void ScorerMSLR::createSmoothing(const vector<double>&  scores, double weight, vector<double>& smoothing) const {
  double total = accumulate(scores.begin(), scores.end(), 0);
  smoothing.push_back(weight*(scores[MONO]+0.1)/total);
  smoothing.push_back(weight*(scores[SWAP]+0.1)/total);
  smoothing.push_back(weight*(scores[DLEFT]+0.1)/total);
  smoothing.push_back(weight*(scores[DRIGHT]+0.1)/total);
}

void ScorerMSLR::createConstSmoothing(double weight, vector<double>& smoothing) const {
  for (int i=1; i<=4; ++i)
    smoothing.push_back(weight);
}


void ScorerMSD::createSmoothing(const vector<double>&  scores, double weight, vector<double>& smoothing) const {
  double total = accumulate(scores.begin(), scores.end(), 0);
  smoothing.push_back(weight*(scores[MONO]+0.1)/total);
  smoothing.push_back(weight*(scores[SWAP]+0.1)/total);
  smoothing.push_back(weight*(scores[DLEFT]+scores[DRIGHT]+scores[OTHER]+0.1)/total);
}

void ScorerMSD::createConstSmoothing(double weight, vector<double>& smoothing) const {
  for (int i=1; i<=3; ++i)
    smoothing.push_back(weight);
}

void ScorerMonotonicity::createSmoothing(const vector<double>&  scores, double weight, vector<double>& smoothing) const {
  double total = accumulate(scores.begin(), scores.end(), 0);
  smoothing.push_back(weight*(scores[MONO]+0.1)/total);
  smoothing.push_back(weight*(scores[SWAP]+scores[DLEFT]+scores[DRIGHT]+scores[OTHER]+scores[NOMONO]+0.1)/total);
}

void ScorerMonotonicity::createConstSmoothing(double weight, vector<double>& smoothing) const {
  for (int i=1; i<=2; ++i)
    smoothing.push_back(weight);
}


void ScorerLR::createSmoothing(const vector<double>&  scores, double weight, vector<double>& smoothing) const {
  double total = accumulate(scores.begin(), scores.end(), 0);
  smoothing.push_back(weight*(scores[MONO]+scores[DRIGHT]+0.1)/total);
  smoothing.push_back(weight*(scores[SWAP]+scores[DLEFT])/total);
}

void ScorerLR::createConstSmoothing(double weight, vector<double>& smoothing) const {
  for (int i=1; i<=2; ++i)
    smoothing.push_back(weight);
}

void Model::score_fe(const string& f, const string& e)  {
  if (!fe) return;    //Make sure we do not do anything if it is not a fe model
	
	outputFile->writeLine(f + " ||| " + e + " ||| ");
	
  //condition on the previous phrase
  if (previous) {
    vector<double> scores;
    scorer->score(modelscore->get_scores_fe_prev(), scores);
    double sum = 0;
    for(size_t i=0; i<scores.size(); ++i) {
      scores[i] += smoothing_prev[i];
      sum += scores[i];
    }
		stringstream out;
		for (size_t i = 0; i < scores.size(); out << scores[i++]/sum << " ");
		outputFile->writeLine(out.str());
  }
  //condition on the next phrase
  if (next) {
    vector<double> scores;
    scorer->score(modelscore->get_scores_fe_next(), scores);
    double sum = 0;
    for(size_t i=0; i<scores.size(); ++i) {
      scores[i] += smoothing_next[i];
      sum += scores[i];
    }
		stringstream out;
		for (size_t i = 0; i < scores.size(); out << scores[i++]/sum << " ");
		outputFile->writeLine(out.str());
  }
	outputFile->writeLine("\n");
}

void Model::score_f(const string& f) {
  if (fe) return;      //Make sure we do not do anything if it is not a f model
	
	outputFile->writeLine(f + " ||| ");
	
  //condition on the previous phrase
  if (previous) {
    vector<double> scores;
    scorer->score(modelscore->get_scores_f_prev(), scores);
    double sum = 0;
    for(int i=0; i<scores.size(); ++i) {
      scores[i] += smoothing_prev[i];
      sum += scores[i];
    }
		stringstream out;
		for (size_t i = 0; i < scores.size(); out << scores[i++]/sum << " ");
		outputFile->writeLine(out.str());
  }
  //condition on the next phrase
  if (next) {
    vector<double> scores;
    scorer->score(modelscore->get_scores_f_next(), scores);
    double sum = 0;
    for(int i=0; i<scores.size(); ++i) {
      scores[i] += smoothing_next[i];
      sum += scores[i];
    }
		stringstream out;
		for (size_t i = 0; i < scores.size(); out << scores[i++]/sum << " ");
		outputFile->writeLine(out.str());
  }
	outputFile->writeLine("\n");
}

Model::Model(ModelScore* ms, Scorer* sc, const string& dir, const string& lang, const string& fn) : modelscore(ms), scorer(sc), outputFile(new Bz2LineWriter(fn)), fe(lang == "fe"), previous(dir != "forward"), next(dir != "backward") {
	if (!fe && lang != "f") {
    cerr << "You have given an illegal language to condition on: "  << lang << endl
		<< "Legal types: fe (on both languages), f (only on source language)" << endl;
    exit(1);
  }
}

Model::~Model() {
	delete outputFile;
  delete modelscore;
  delete scorer;
}

Model* Model::createModel(ModelScore* modelscore, string config, const string& filepath) {
	//Save the filename first
  string filename = filepath == "-" ? filepath : filepath + "." + config + ".bz2";
	//Take out the type
	config = config.substr(config.find('-') + 1);
	//Read the orientation
	string orientation = config.substr(0, config.find('-'));
	config = config.substr(config.find('-') + 1);
	//Read the direction
	string direction = config.substr(0, config.find('-'));
	//Read the language
	string language = config.substr(config.find('-') + 1);

  if (orientation == "mslr")
    return new Model(modelscore, new ScorerMSLR(), direction, language, filename);
	else if (orientation == "msd")
    return new Model(modelscore, new ScorerMSD(), direction, language, filename);
	else if (orientation == "monotonicity")
    return new Model(modelscore, new ScorerMonotonicity(), direction, language, filename);
	else if (orientation == "leftright")
    return new Model(modelscore, new ScorerLR(), direction, language, filename);
	else {
    cerr << "Illegal orientation type of reordering model: " << orientation << endl << " allowed types: mslr, msd, monotonicity, leftright" << endl;
    exit(1);
  }
}



void Model::createSmoothing(double w)  {
  scorer->createSmoothing(modelscore->get_scores_fe_prev(), w, smoothing_prev);
  scorer->createSmoothing(modelscore->get_scores_fe_next(), w, smoothing_next);
}

void Model::createConstSmoothing(double w)  {
  scorer->createConstSmoothing(w, smoothing_prev);
  scorer->createConstSmoothing(w, smoothing_next);
}
