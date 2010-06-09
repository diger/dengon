//////////////////////////////////////////////////
// Blabber [RosterItem.h]
//     Entries of the RosterView widget
//////////////////////////////////////////////////

#ifndef ROSTER_ITEM_H
#define ROSTER_ITEM_H

#ifndef _BITMAP_H
	#include <interface/Bitmap.h>
#endif

#ifndef _LIST_ITEM_H
	#include <interface/ListItem.h>
#endif

#ifndef _VIEW_H
	#include <interface/View.h>
#endif

#ifndef USER_ID_H
	#include "UserID.h"
#endif

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
		
	static BBitmap *_kinda_online_icon;
	static BBitmap *_online_icon;
	static BBitmap *_offline_icon;
	static BBitmap *_unknown_icon;
	static BBitmap *_aol_icon;
	static BBitmap *_icq_icon;
	static BBitmap *_yahoo_icon;
	static BBitmap *_msn_icon;
};

#endif