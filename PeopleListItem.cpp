//////////////////////////////////////////////////
// Jabber [PeopleListItem.cpp]
//////////////////////////////////////////////////

#include "PeopleListItem.h"
#include <interface/Font.h>
#include <interface/View.h>
#include "JabberProtocol.h"

PeopleListItem::PeopleListItem(std::string whoami, std::string user)
	: BListItem() {
	_user   = user;
	_whoami = whoami;
}

PeopleListItem::~PeopleListItem() {
}

void PeopleListItem::DrawItem(BView *owner, BRect frame, bool complete) {
	// text characteristics
	owner->SetFont(be_plain_font);
	owner->SetFontSize(11.0);

	// clear rectangle
	if (IsSelected()) {
		if (User() == _whoami) {
			owner->SetHighColor(255, 200, 200);
		} else {
			owner->SetHighColor(200, 200, 255);
		}

		owner->SetLowColor(owner->HighColor());
	} else {
		owner->SetHighColor(owner->ViewColor());
		owner->SetLowColor(owner->HighColor());
	}

	owner->FillRect(frame);

	// construct text positioning
	font_height fh;
	owner->GetFontHeight(&fh);

	float height = fh.ascent + fh.descent;

	// standard text color
	if (User() == _whoami) {
		owner->SetHighColor(255, 0, 0);
	} else {
		owner->SetHighColor(0, 0, 255);
	}

	// draw information
	owner->DrawString(User().c_str(), BPoint(frame.left + 5.0, frame.bottom - ((frame.Height() - height) / 2) - fh.descent));
}

void PeopleListItem::Update(BView *owner, const BFont *font) {
	BListItem::Update(owner, font);

	// set height to accomodate graphics and text
	SetHeight(13.0);
}

std::string PeopleListItem::User() const {
	return _user;
}
