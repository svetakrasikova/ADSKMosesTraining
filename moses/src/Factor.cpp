// $Id: Factor.cpp 1897 2008-10-08 23:51:26Z hieuhoang1972 $

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh
© 2010 Ventsislav Zhechev
Last modified by Ventsislav Zhechev on 19 Apr 2010

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

#include "Factor.h"

using namespace std;

namespace Moses
{
Factor::Factor(FactorDirection direction, FactorType factorType, const std::string *factorString, size_t id)
://m_direction(direction)
//,m_factorType(factorType)
m_ptrString(factorString)
,m_id(id)
{}

Factor::Factor(FactorDirection direction, FactorType factorType, const std::string *factorString)
//:m_direction(direction)
//,m_factorType(factorType)
:m_ptrString(factorString)
,m_id(NOT_FOUND)
{}

TO_STRING_BODY(Factor)

// friend
ostream& operator<<(ostream& out, const Factor& factor)
{
	out << factor.GetString();
	return out;
}

	wostream& operator<<(wostream& out, const Factor& factor) {
		out << factor.GetString().c_str();
		return out;
	}
	
}


