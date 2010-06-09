//////////////////////////////////////////////////
// Blabber [RosterSuperitem.h]
//     Entries of the RosterView widget that
//     maintain the RosterItems
//////////////////////////////////////////////////

#ifndef ROSTER_SUPERITEM_H
#define ROSTER_SUPERITEM_H

#ifndef _LIST_ITEM_H
	#include <interface/ListItem.h>
#endif

#ifndef _VIEW_H
	#include <interface/View.h>
#endif

class RosterSuperitem : public BStringItem {
public:
			       RosterSuperitem(const char *text);
  			      ~RosterSuperitem();

	void           DrawItem(BView *owner, BRect frame, bool complete = false);
};

#endif