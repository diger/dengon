//////////////////////////////////////////////////
// Jabber [PeopleListItem.h]
//     A listing of an existing user
//////////////////////////////////////////////////

#ifndef PEOPLE_LIST_ITEM_H
#define PEOPLE_LIST_ITEM_H

#include <string>
#include <interface/ListItem.h>


class PeopleListItem : public BListItem {
public:
	              PeopleListItem(std::string whoami, std::string user,
	              	std::string show, std::string status, std::string role, std::string affiliation);
   	             ~PeopleListItem();

	virtual void  DrawItem(BView *owner, BRect rect, bool complete);
	virtual void  Update(BView *owner, const BFont *font);

	std::string   User() const;

private:
	std::string   _user;
	std::string   _whoami;
	string		_show;
	string		_status;
	string		_affiliation;
	string		_role;
};

#endif

