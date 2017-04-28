#include "SimpleID.hpp"

SimpleID::SimpleID() :
	nextID(1)
{ }

idnum SimpleID::getID()
{
	idnum saveID;
	if (nextID == 0)  nextID = 1;
	saveID = nextID;
	nextID++;
	return saveID;
}