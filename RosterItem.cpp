//////////////////////////////////////////////////
// Blabber [RosterItem.cpp]
//////////////////////////////////////////////////

#include "RosterItem.h"
#include "AppLocation.h"
#include <TranslationUtils.h>

BBitmap *RosterItem::_kinda_online_icon = NULL;
BBitmap *RosterItem::_offline_icon      = NULL;
BBitmap *RosterItem::_online_icon       = NULL;
BBitmap *RosterItem::_unknown_icon      = NULL;
BBitmap *RosterItem::_aol_icon          = NULL;
BBitmap *RosterItem::_icq_icon          = NULL;
BBitmap *RosterItem::_msn_icon          = NULL;
BBitmap *RosterItem::_yahoo_icon        = NULL;

RosterItem::RosterItem(const UserID *userid)
	: BStringItem(userid->FriendlyName().c_str()) {
	_userid           = userid;
	_is_stale_pointer = false;
/*
	// intitialize static members
	if (_offline_icon == NULL) {
		std::string graphic;

		// generate path for home icon
		graphic = AppLocation::Instance()->AbsolutePath("resources/icons/away-online.png");

		// load graphic
		_kinda_online_icon = BTranslationUtils::GetBitmap(graphic.c_str());

		// generate path for home icon
		graphic = AppLocation::Instance()->AbsolutePath("resources/icons/online.png");

		// load graphic
		_online_icon = BTranslationUtils::GetBitmap(graphic.c_str());

		// generate path for away icon
		graphic = AppLocation::Instance()->AbsolutePath("resources/icons/offline.png");

		// load graphic
		_offline_icon = BTranslationUtils::GetBitmap(graphic.c_str());

		// generate path for away icon
		graphic = AppLocation::Instance()->AbsolutePath("resources/icons/unknown.png");

		// load graphic
		_unknown_icon = BTranslationUtils::GetBitmap(graphic.c_str());

		// generate path for AOL icon
		graphic = AppLocation::Instance()->AbsolutePath("resources/icons/aol.png");

		// load graphic
		_aol_icon = BTranslationUtils::GetBitmap(graphic.c_str());

		// generate path for ICQ icon
		graphic = AppLocation::Instance()->AbsolutePath("resources/icons/icq.png");

		// load graphic
		_icq_icon = BTranslationUtils::GetBitmap(graphic.c_str());

		// generate path for Yahoo! icon
		graphic = AppLocation::Instance()->AbsolutePath("resources/icons/yahoo.png");

		// load graphic
		_yahoo_icon = BTranslationUtils::GetBitmap(graphic.c_str());

		// generate path for MSN icon
		graphic = AppLocation::Instance()->AbsolutePath("resources/icons/msn.png");

		// load graphic
		_msn_icon = BTranslationUtils::GetBitmap(graphic.c_str());
	}
	*/
}

RosterItem::~RosterItem() {
}

void RosterItem::DrawItem(BView *owner, BRect frame, bool complete)
{
	// protection
	if (StalePointer()) {
		return;
	}

	// get online status
	UserID::online_status status = _userid->OnlineStatus();
	std::string exact_status = _userid->ExactOnlineStatus();

	// text characteristics
	owner->SetFont(be_plain_font);
	owner->SetFontSize(10.0);
	
				//owner->GetFont(&statusFont);
				//statusFont.SetFace(B_ITALIC_FACE);
				//owner->SetFont(&statusFont);

	// clear rectangle
	if (IsSelected())
	{
		// font color is based on online status
		if (status == UserID::ONLINE)
		{
			if (exact_status == "xa") 		 owner->SetHighColor(255, 220, 220, 255);
			else if (exact_status == "away") owner->SetHighColor(255, 230, 210, 255); 
			else if (exact_status == "dnd")  owner->SetHighColor(255, 192, 192, 255); 
			else 							 owner->SetHighColor(192, 255, 192, 255); 
		}
		else if (status == UserID::OFFLINE)
		{
			owner->SetHighColor(220, 220, 220, 255);
		}
		else
		{
			owner->SetHighColor(173, 216, 230, 255);
		}
	}
	else
	{
		owner->SetHighColor(owner->ViewColor());
	}

	owner->FillRect(frame);

	float height;

	// construct name
	std::string name = GetUserID()->FriendlyName();

	if (name.empty()) {
		name = GetUserID()->Handle();

		if (name.empty()) {
			name = "<anonymous>";
		}
	}

	BFont statusFont;
			
	// font color is based on online status
	if (status == UserID::ONLINE)
	{
		if (exact_status == "xa") 			owner->SetHighColor(139, 0, 0, 255);
		else if (exact_status == "away")	owner->SetHighColor(255, 140, 0, 255);
		else if (exact_status == "dnd")		owner->SetHighColor(255, 0, 0, 255);
		else								owner->SetHighColor(0, 100, 0, 255);
	}
	else if (status == UserID::OFFLINE)
	{
		owner->SetHighColor(90, 90, 90, 255); // gray
	}
	else
	{
		owner->SetHighColor(0, 0, 255, 255); // blue
	}

	// construct text positioning
	font_height fh;
	owner->GetFontHeight(&fh);

	height = fh.ascent + fh.descent;

	// draw name
	owner->DrawString(name.c_str(),
		BPoint(frame.left/* + 13*/, frame.bottom - ((frame.Height() - height) / 2) - fh.descent));

	// draw show
	if (!GetUserID()->MoreExactOnlineStatus().empty()) {
		owner->SetHighColor(0, 0, 0, 255);

		owner->DrawString(" ");
		owner->DrawString(GetUserID()->MoreExactOnlineStatus().c_str());
		owner->DrawString("");
	}
/*
	// draw external chat icon
	if (GetUserID()->UserType() == UserID::AIM) {
		if (_aol_icon) {
			owner->DrawBitmapAsync(_aol_icon, BPoint(owner->PenLocation().x + 2.0, frame.top + 2));
		}
	} else if (GetUserID()->UserType() == UserID::YAHOO) {
		if (_yahoo_icon) {
			owner->DrawBitmapAsync(_yahoo_icon, BPoint(owner->PenLocation().x + 3.0, frame.top + 3));
		}
	} else if (GetUserID()->UserType() == UserID::ICQ) {
		if (_icq_icon) {
			owner->DrawBitmapAsync(_icq_icon, BPoint(owner->PenLocation().x + 2.0, frame.top + 2));
		}
	} else if (GetUserID()->UserType() == UserID::MSN) {
		if (_msn_icon) {
			owner->DrawBitmapAsync(_msn_icon, BPoint(owner->PenLocation().x + 2.0, frame.top + 2));
		}
	}
*/	
	owner->SetFont(be_plain_font);
	owner->SetFontSize(10.0);

}

void RosterItem::Update(BView *owner, const BFont *font) {
	BListItem::Update(owner, font);

	// set height to accomodate graphics and text
	SetHeight(16.0);
}

bool RosterItem::StalePointer() const {
	return _is_stale_pointer;
}

const UserID *RosterItem::GetUserID() const {
	if (StalePointer()) {
		return NULL;
	} else {
		return _userid;
	}
}

void RosterItem::SetStalePointer(bool is_stale) {
	_is_stale_pointer = is_stale;
}
