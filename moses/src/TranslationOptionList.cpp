/*
 *
 *      Creater unknown
 *			© 2010 Autodesk Development Sàrl
 *			Last modified by Ventsislav Zhechev on 21 Jun 2010
 *
 */
#include "TranslationOptionList.h"
#include "Util.h"
#include "TranslationOption.h"

using namespace std;

namespace Moses
{

TranslationOptionList::TranslationOptionList(const TranslationOptionList &copy)
{
	const_iterator iter;
	for (iter = copy.begin(); iter != copy.end(); ++iter)
	{
		const TranslationOption &origTransOpt = **iter;
		TranslationOption *newTransOpt = new TranslationOption(origTransOpt);
		Add(newTransOpt);
	}
}

TranslationOptionList::~TranslationOptionList()
{
//	cerr << "™™ Deleting the following translation options: " << endl << *this << endl;
	RemoveAllInColl(m_coll);
}

TO_STRING_BODY(TranslationOptionList);

std::ostream& operator<<(std::ostream& out, const TranslationOptionList& coll)
{
	TranslationOptionList::const_iterator iter;
	for (iter = coll.begin(); iter != coll.end(); ++iter)
	{
		const TranslationOption &transOpt = **iter;
		out << "@" << *iter << "/" << transOpt << endl;
	}
	
	return out;
}

} // namespace
