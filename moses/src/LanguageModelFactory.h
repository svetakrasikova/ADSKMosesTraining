// $Id: LanguageModelFactory.h 2939 2010-02-24 11:15:44Z jfouet $

#ifndef moses_LanguageModelFactory_h
#define moses_LanguageModelFactory_h

#include <string>
#include <vector>
#include "TypeDef.h"

namespace Moses
{

class LanguageModel;
class ScoreIndexManager;

namespace LanguageModelFactory {

	/**
	 * creates a language model that will use the appropriate
   * language model toolkit as its underlying implementation
	 */
	 LanguageModel* CreateLanguageModel(LMImplementation lmImplementation
																		, const std::vector<FactorType> &factorTypes     
																		, size_t nGramOrder
																		, const std::string &languageModelFile
																		, float weight
																		, ScoreIndexManager &scoreIndexManager
																		, int dub);
	 
};

}

#endif
