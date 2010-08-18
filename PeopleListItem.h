//////////////////////////////////////////////////
// Jabber [PeopleListItem.h]
//     A listing of an existing user
//////////////////////////////////////////////////

#ifndef PEOPLE_LIST_ITEM_H
#define PEOPLE_LIST_ITEM_H

#ifndef __STRING__
	#include <string>
#endif

#ifndef LIST_ITEM_H
	#include <interface/ListItem.h>
#endif

class PeopleListItem : public BListItem {
public:
	              PeopleListItem(std::string whoami, std::string user);
   	             ~PeopleListItem();

	virtual void  DrawItem(BView *owner, BRect rect, bool complete);
	virtual void  Update(BView *owner, const BFont *font);

	std::string   User() const;

private:
	std::string   _user;
	std::string   _whoami;
};

#endif

