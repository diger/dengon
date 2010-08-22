//////////////////////////////////////////////////
// Blabber [RosterItem.h]
//     Entries of the RosterView widget
//////////////////////////////////////////////////

#ifndef ROSTER_ITEM_H
#define ROSTER_ITEM_H

#include <interface/ListItem.h>
#include <interface/View.h>
#include "UserID.h"

class RosterItem : public BStringItem {
public:
			         RosterItem(const UserID *userid);
  			        ~RosterItem();

	void             DrawItem(BView *owner, BRect frame, bool complete = false);
	virtual void     Update(BView *owner, const BFont *font);
	
	bool             StalePointer() const;

	const UserID    *GetUserID() const;
	void             SetStalePointer(bool is_stale);	
	
private:
	const UserID   *_userid;
	bool            _is_stale_pointer;
};

#endif
