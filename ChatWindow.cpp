#include "ChatWindow.h"
#include "SplitView.h"
#include "StatusView.h"
#include "Settings.h"
#include "Messages.h"
#include "EditingFilter.h"
#include "GenericFunctions.h"
#include "JabberProtocol.h"
#include "TalkManager.h"
#include "PeopleListItem.h"

#include <malloc.h>
#include <stdlib.h>
#include <Font.h>
#include <Application.h>
#include <Menu.h>
#include <MenuItem.h>
#include <View.h>
#include <interface/TextView.h>
#include <String.h>
#include <string.h>
#include <FindDirectory.h>
#include <storage/Path.h>

void ChatWindow::SetThreadID(string id)
{
	_thread = id;
}

const UserID *ChatWindow::GetUserID()
{
	return _user;
}

string ChatWindow::GetGroupRoom()
{
	return _group_room;
}

string ChatWindow::GetGroupUsername()
{
	return _group_username;
}

ChatWindow::ChatWindow(talk_type type, UserID *user, std::string group_room,
				std::string group_username)
	:BWindow(BRect(100,100,500,400),"Travis",B_DOCUMENT_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	_chat_index = -1;
	
	//MessageRepeater::Instance()->AddTarget(this);

	_type           = type;
	_user           = user;
	_group_room     = group_room;
	_group_username = group_username;
	
	if (_type != ChatWindow::GROUP) {
		_current_status = user->OnlineStatus();
	}
	
	_thread = GenericFunctions::GenerateUniqueID();
	
	 bool bAutoOpenChatLog = BlabberSettings::Instance()->Tag("autoopen-chatlog");
	string chatlog_path = "";
	if (BlabberSettings::Instance()->Data("chatlog-path") != NULL) {
		chatlog_path = BlabberSettings::Instance()->Data("chatlog-path");
	}
	if(bAutoOpenChatLog) {
		if(0 == chatlog_path.size()) {
			BPath path;
			find_directory(B_USER_DIRECTORY, &path);
			chatlog_path = path.Path();
		}
		// assure that directory exists...
		create_directory(chatlog_path.c_str(), 0777);
		if(_user != 0) {
		  chatlog_path += "/" + _user->JabberHandle();
		} else {
		  chatlog_path += "/" + group_room;
		}	
		// start file
		_log = fopen(chatlog_path.c_str(), "a");
		_am_logging = (0 != _log);
	}
	
	BRect b = Bounds();
	BRect ori = b;
	float statusHeight = 12;
	float splitterHeight = 4;//statusHeight + 1;
	float menuHeight = 18;
	float split = 51;

	// Menu

	menu = new BMenuBar(b, NULL);
		
	// Main View

	mainView = new BView(b, "TravisView", B_FOLLOW_ALL_SIDES, 0);
	mainView->SetViewColor(216,216,216);
	
		// History View
	
		BRect text_rect(ori);
		text_rect.left += 2;
    	historyTextView = new BTextView(b, "history", text_rect, B_FOLLOW_ALL | B_FRAME_EVENTS);
		historyScroller = new BScrollView("history_croller", historyTextView, B_FOLLOW_ALL, false, true);
		historyTextView->TargetedByScrollView(historyScroller);
		historyTextView->SetFontSize(12.0);
		historyTextView->SetText("");
		historyTextView->SetWordWrap(true);
		historyTextView->SetStylable(true);
		historyTextView->MakeEditable(false);
		b.top = b.bottom;
		b.left = b.right + 2;
		BView *historyView = new BView(b, NULL, B_FOLLOW_ALL, 0);
		historyView->AddChild(historyScroller);
		
		// Group Chat People View

		BView *peopleView;
		if (_type == GROUP)
		{
			BRect people_rect = Bounds();
			_people = new BListView(people_rect, NULL, B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL);
			_scrolled_people_pane = new BScrollView(NULL, _people, B_FOLLOW_ALL, false, true);
			peopleView = new BView(b, NULL, B_FOLLOW_ALL, 0);
			peopleView->AddChild(_scrolled_people_pane);	
			_split_group_people = new SplitPane(people_rect, historyView, peopleView, B_FOLLOW_ALL);
			_split_group_people->SetAlignment(B_VERTICAL);
			_split_group_people->SetMinSizeOne(300.0);
			_split_group_people->SetMinSizeTwo(100.0);
			_split_group_people->SetBarThickness(splitterHeight);
			_split_group_people->SetBarAlignmentLocked(true);
			_split_group_people->SetBarPosition(400);

		}	

		// Message View
	
		BRect text_rect2(ori);
		text_rect2.left += 2;
    	messageTextView = new BTextView(b, "message", text_rect2, B_FOLLOW_ALL | B_FRAME_EVENTS);
		messageScroller = new BScrollView("message_scroller", messageTextView, B_FOLLOW_ALL, false, true);
		messageTextView->TargetedByScrollView(messageScroller);
		messageTextView->SetFontSize(12.0);
		messageTextView->SetWordWrap(true);
		messageTextView->SetText("");
		messageTextView->SetStylable(true);
		messageTextView->MakeEditable(true);
		b.top = b.bottom;
		b.left = b.right + 2;
		messageView = new BView(b, NULL, B_FOLLOW_ALL, 0);
		messageView->AddChild(messageScroller);
	
		// Main Horizontal Split View (Chat Window View)
		
		ori.top += menuHeight;
		ori.bottom -= statusHeight;
		ori.left -= 2;
		if (_type == GROUP)
			chatView = new SplitPane(ori, _split_group_people, messageView, B_FOLLOW_TOP_BOTTOM | B_FOLLOW_LEFT_RIGHT);
		else
			chatView = new SplitPane(ori, historyView, messageView, B_FOLLOW_TOP_BOTTOM | B_FOLLOW_LEFT_RIGHT);
		chatView->SetAlignment(B_HORIZONTAL);
		chatView->SetBarThickness(splitterHeight);
		chatView->SetBarAlignmentLocked(true);
		chatView->SetMinSizeTwo(split);
		chatView->SetMinSizeOne(split);
		chatView->SetBarPosition(ori.bottom);
		
		
		
	// Status View
	
	statusView = new StatusView();
	statusView->SetViewColor(216, 216, 216, 255);
	statusView->SetLowColor(216, 216, 216, 255);
	std::string statusMessage = "online";
	statusView->SetMessage(statusMessage);
	
	// group chat people list
	

	
	AddCommonFilter(new EditingFilter(messageTextView, this));
	
	mainView->AddChild(menu);
	mainView->AddChild(chatView);
	mainView->AddChild(statusView);
	AddChild(mainView);
	
	messageTextView->MakeFocus(true);
	
	if (user->FriendlyName().size())
	{
		SetTitle(user->FriendlyName().c_str());
		originalWindowTitle.SetTo(user->FriendlyName().c_str());
	}
	else
	{
		SetTitle(user->JabberHandle().c_str());
		originalWindowTitle.SetTo(user->JabberHandle().c_str());
	}
	
	Show();
	
	if (_type != GROUP)
		fprintf(stderr, "Show Chat Window %s.\n", user->JabberCompleteHandle().c_str());
	else
		fprintf(stderr, "Show Group Chat Window Room %s Username %s.\n", group_room.c_str(), group_username.c_str());
		
	
}

/*
ChatWindow::ChatWindow(void)
	:	BWindow(BRect(100,100,500,400),"Travis",B_DOCUMENT_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	
   
}
*/

void
ChatWindow::FrameResized(float width, float height)
{
	
	BWindow::FrameResized(width, height);

	BRect chat_rect = historyTextView->Frame();
	BRect message_rect = messageTextView->Frame();

	chat_rect.OffsetTo(B_ORIGIN);
	message_rect.OffsetTo(B_ORIGIN);
	
	chat_rect.InsetBy(2.0, 2.0);
	message_rect.InsetBy(2.0, 2.0);
	
	historyTextView->SetTextRect(chat_rect);
	messageTextView->SetTextRect(message_rect);

	historyTextView->Invalidate();
	historyScroller->Invalidate();
	
	// remember sizes of message windows
	//if (_type == TalkWindow::MESSAGE) {
	//	BlabberSettings::Instance()->SetFloatData("message-window-width", width);
	//	BlabberSettings::Instance()->SetFloatData("message-window-height", height);
	//}
}

void
ChatWindow::NewMessage(string new_message)
{
	//if (_type == ChatWindow::GROUP) {
	//	return; // GCHAT
	//} else {
		if (!_user->FriendlyName().empty()) {
			AddToTalk(_user->FriendlyName(), new_message, MAIN_RECIPIENT);
		} else {
			AddToTalk(_user->JabberUsername(), new_message, MAIN_RECIPIENT);
		}
	//}
}

void
ChatWindow::NewMessage(string username, string new_message)
{
	AddToTalk(username, new_message, MAIN_RECIPIENT);
}

void
ChatWindow::AddToTalk(string username, string message, user_type type)
{
	BFont thin(be_plain_font);
	BFont thick(be_bold_font);

	thin.SetSize(12.0);
	thick.SetSize(12.0);
			
	// some colors to play with
	rgb_color blue   = {0, 0, 255, 255};
	rgb_color red    = {255, 0, 0, 255};
	rgb_color black  = {0, 0, 0, 255};
	
	// some runs to play with
	text_run tr_thick_blue  = {0, thick, blue};
	text_run tr_thick_red   = {0, thick, red};
	text_run tr_thick_black = {0, thick, black};
	text_run tr_thin_black  = {0, thin, black};

	// some run array to play with (simple)
	text_run_array tra_thick_blue  = {1, {tr_thick_blue}}; 
	text_run_array tra_thick_red   = {1, {tr_thick_red}}; 
	text_run_array tra_thick_black = {1, {tr_thick_black}}; 
	text_run_array tra_thin_black  = {1, {tr_thin_black}}; 
	
	if (historyTextView == NULL) return;
	
	if (type == MAIN_RECIPIENT)
		historyTextView->Insert(historyTextView->TextLength(), username.c_str(), username.length(), &tra_thick_red);
	else
		historyTextView->Insert(historyTextView->TextLength(), username.c_str(), username.length(), &tra_thick_blue);
		
	historyTextView->Insert(historyTextView->TextLength(), ": ", 2, &tra_thick_black);
	historyTextView->Insert(historyTextView->TextLength(), message.c_str(), message.length(), &tra_thin_black);
	historyTextView->Insert(historyTextView->TextLength(), "\n", 1, &tra_thin_black);
	
	historyTextView->ScrollTo(0.0, historyTextView->Bounds().bottom);
}

void
ChatWindow::AddGroupChatter(string user)
{
	int i;

	// create a new entry
	PeopleListItem *people_item = new PeopleListItem(_group_username, user);
	
	// exception
	if (_people->CountItems() == 0) {
		// add the new user
		_people->AddItem(people_item);

		return;
	}

	// add it to the list
	for (i=0; i < _people->CountItems(); ++i) {
		PeopleListItem *iterating_item = dynamic_cast<PeopleListItem *>(_people->ItemAt(i));

		if (strcasecmp(iterating_item->User().c_str(), user.c_str()) > 0) {
			// add the new user
			_people->AddItem(people_item, i);
			break;
		} else if (!strcasecmp(iterating_item->User().c_str(), user.c_str()) && strcmp(iterating_item->User().c_str(), user.c_str()) > 0) {
			// add the new user
			_people->AddItem(people_item, i);
			break;
		} else if (!strcasecmp(iterating_item->User().c_str(), user.c_str()) && !strcmp(iterating_item->User().c_str(), user.c_str())) {
			_people->InvalidateItem(i);
			break;
		} else if (!strcasecmp(iterating_item->User().c_str(), user.c_str()) && strcmp(iterating_item->User().c_str(), user.c_str()) < 0) {
			if (i == (_people->CountItems() - 1)) {
				// add the new user
				_people->AddItem(people_item);
			} else {
				PeopleListItem *next_item = dynamic_cast<PeopleListItem *>(_people->ItemAt(i + 1));
				
				if (!next_item) {
					// add the new user
					_people->AddItem(people_item);

					break;					
				}

				if (strcasecmp(user.c_str(), next_item->User().c_str()) < 0) {
					// add the new user
					_people->AddItem(people_item, i + 1);
				} else if (!strcasecmp(user.c_str(), next_item->User().c_str())) {
					continue;
				}
			}

			break;
		} else if ((strcasecmp(iterating_item->User().c_str(), user.c_str()) < 0) && (i == (_people->CountItems() - 1))) {
			// add the new user
			_people->AddItem(people_item);

			break;
		} else if (strcasecmp(iterating_item->User().c_str(), user.c_str()) < 0) {
			continue;
		}
	}
}

void
ChatWindow::RemoveGroupChatter(string username)
{
	// remove user
	for (int i=0; i < _people->CountItems(); ++i) {
		if (dynamic_cast<PeopleListItem *>(_people->ItemAt(i))->User() == username) {
			_people->RemoveItem(i);
		}
	}
}

BString
ChatWindow::OurRepresentation()
{
	if (jabber == NULL) return "MisterX";
	BString representation;
	representation << jabber->user << "@" << jabber->domain;
	return representation;
}



void
ChatWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case JAB_CHAT_SENT:
		{
			const char *messageTextANSI = messageTextView->Text();
			string messageTextSTL = messageTextView->Text();
			BString message = BString(messageTextANSI);
			string s = OurRepresentation().String();
			AddToTalk(s, messageTextSTL, LOCAL);
			messageTextView->SetText("");
			messageTextView->MakeFocus(true);
			if (_type == GROUP)
				jabber->SendGroupchatMessage(BString(_user->JabberHandle().c_str()), message);
			else
				jabber->SendMessage(BString(_user->JabberHandle().c_str()), message);

			break;
		}
		
		case JAB_GROUP_CHATTER_ONLINE:
		{
			// only for groupchat
			if (_type != GROUP) {
				break;
			}

			if (GetGroupRoom() == msg->FindString("room"))
			{
				AddGroupChatter(msg->FindString("username"));
			}
			
			break;
		}

		case JAB_GROUP_CHATTER_OFFLINE:
		{
			// only for groupchat
			if (_type != GROUP) {
				break;
			}

			RemoveGroupChatter(msg->FindString("username"));
			
			break;
		}
	
		default:
		{
			BWindow::MessageReceived(msg);
			break;
		}
	}
}

bool
ChatWindow::QuitRequested(void)
{
	//be_app->PostMessage(B_QUIT_REQUESTED);
	TalkManager::Instance()->RemoveWindow(_user->JabberHandle());
	return true;
}

