#include "ChatWindow.h"
#include "SplitView.h"
#include "StatusView.h"
#include "Settings.h"
#include "Messages.h"
#include "MessageRepeater.h"
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
	
	MessageRepeater::Instance()->AddTarget(this);

	_type           = type;
	_user           = new UserID(*user);
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
			_split_group_people->SetMinSizeOne(50.0);
			_split_group_people->SetMinSizeTwo(120.0);
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
		fprintf(stderr, "Show Group Chat Window Room %s Username %s.\n", 
			group_room.c_str(),
			group_username.c_str());
		
	
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
	if (!_user->FriendlyName().empty()) {
		AddToTalk(_user->FriendlyName(), new_message, MAIN_RECIPIENT);
	} else {
		AddToTalk(_user->JabberUsername(), new_message, MAIN_RECIPIENT);
	}
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
	
	if (message.substr(0,4) == "/me ")
	{
		// print action
		string action = "* " + username + " " + message.substr(4) + "\n";  
		historyTextView->Insert(historyTextView->TextLength(), action.c_str(), action.length(), &tra_thick_blue);
	}
	/*
	else if (message.substr(0,6) == "/join ")
	{
		string room = message.substr(6);
		UserID userid(room);
		
		ChatWindow *window = 
						TalkManager::Instance()->CreateTalkSession(
							ChatWindow::GROUP, &userid, userid.JabberHandle(),
								string(jabber->user) //+ '@' + string(jabber->domain)
								);
								
		//jabber->SendGroupPresence(room, string(jabber->user.String()));
		//JoinRoom(BString((room + '/' + string(jabber->user)).c_str()), "");
	}
	*/
	else
	{
		// print message
		if (last_username.empty() || last_username != username)
		{
			if (type == MAIN_RECIPIENT)
				historyTextView->Insert(historyTextView->TextLength(),
					username.c_str(), username.length(), &tra_thick_red);
			else
				historyTextView->Insert(historyTextView->TextLength(),
					username.c_str(), username.length(), &tra_thick_blue);
		
			historyTextView->Insert(historyTextView->TextLength(), ": ", 2, &tra_thick_black);
		}

		text_run_array *this_array;
		GenerateHyperlinkText(message, tr_thin_black, &this_array);
		historyTextView->Insert(historyTextView->TextLength(), message.c_str(), message.size(), this_array);
		free(this_array);
		historyTextView->Insert(historyTextView->TextLength(), "\n", 1, &tra_thin_black);
	}
	
	historyTextView->ScrollTo(0.0, historyTextView->Bounds().bottom);
	
	last_username = username;
}

void
ChatWindow::AddGroupChatter(string user, string show, string status, string role, string affiliation)
{
	int i;

	// create a new entry
	PeopleListItem *people_item = new PeopleListItem(_group_username, user, show, status, role, affiliation);
	
	// exception
	if (_people->CountItems() == 0) {
		// add the new user
		_people->AddItem(people_item);

		return;
	}

	// add it to the list
	for (i=0; i < _people->CountItems(); ++i)
	{
		PeopleListItem *iterating_item = dynamic_cast<PeopleListItem *>(_people->ItemAt(i));
		

		if (strcasecmp(iterating_item->User().c_str(), user.c_str()) > 0) {
			// add the new user
			_people->AddItem(people_item, i);
			break;
		} else if (!strcasecmp(iterating_item->User().c_str(), user.c_str()) &&
					strcmp(iterating_item->User().c_str(), user.c_str()) > 0) {
			// add the new user
			_people->AddItem(people_item, i);
			break;
		} else if (!strcasecmp(iterating_item->User().c_str(), user.c_str()) &&
				   !strcmp(iterating_item->User().c_str(), user.c_str()))
		{

			iterating_item->_show = show;
			iterating_item->_status = status;
			iterating_item->_role = role;
			iterating_item->_affiliation = role;
			
			_people->InvalidateItem(i);
			break;
		} else if (!strcasecmp(iterating_item->User().c_str(), user.c_str()) &&
					strcmp(iterating_item->User().c_str(), user.c_str()) < 0) {
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
		} else if ((strcasecmp(iterating_item->User().c_str(), user.c_str()) < 0) &&
					(i == (_people->CountItems() - 1))) {
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

int ChatWindow::CountHyperlinks(string message) {
	string::size_type curr_pos = 0, link_start, link_end;
	string::size_type find1, find2, find3;
	
	// keep count
	int link_count = 0;
	
	// find next link
	link_start = message.find("http://", curr_pos);

	find1 = message.find("ftp://", curr_pos);
	if (find1 != string::npos && (link_start == string::npos || find1 < link_start)) {
		link_start = find1;
	}

	find2 = message.find("www.", curr_pos);
	if (find2 != string::npos && (link_start == string::npos || find2 < link_start)) {
		// ignore if it's not at the beginning or has no whitespace
		if ((find2 - 1) >= 0 && isalnum(message[find2 - 1])) {
			// do nothing
 		} else if (isspace(message[find2 + 4]) || message[find2 + 4] == '.') {
			// do nothing
		} else {
			link_start = find2;
		}
	}

	find3 = message.find("ftp.", curr_pos);
	if (find3 != string::npos && (link_start == string::npos || find3 < link_start)) {
		// ignore if it's not at the beginning or has no whitespace
		if ((find3 - 1) >= 0 && isalnum(message[find3 - 1])) {
			// do nothing
 		} else if (isspace(message[find3 + 4]) || message[find3 + 4] == '.') {
			// do nothing
		} else {
			link_start = find3;
		}
	}

	while (link_start != string::npos) {
		// find whitespace or end
		link_end = message.find_first_of(" \t\r\n", link_start);

		if (link_end == string::npos) {
			link_end = message.size() - 1;
		}

		// prune punctuation
		while (link_start < link_end) {
			if (message[link_end] == ',' || message[link_end] == '!' || message[link_end] == '.' || message[link_end] == ')' || message[link_end] == ';' || message[link_end] == ']' || message[link_end] == '>' || message[link_end] == '\'' || message[link_end] == '"') {
				--link_end;
			} else {
				break;
			}
		}
		
		if (link_start < link_end) {
			++link_count;
		}
		
		curr_pos = link_end + 1;
		
		// find next link
		link_start = message.find("http://", curr_pos);

		find1 = message.find("ftp://", curr_pos);
		if (find1 != string::npos && (link_start == string::npos || find1 < link_start)) {
			link_start = find1;
		}

		find2 = message.find("www.", curr_pos);
		if (find2 != string::npos && (link_start == string::npos || find2 < link_start)) {
			// ignore if it's not at the beginning or has no whitespace
			if ((find2 - 1) >= 0 && isalnum(message[find2 - 1])) {
				// do nothing
	 		} else if (isspace(message[find2 + 4]) || message[find2 + 4] == '.') {
				// do nothing
			} else {
				link_start = find2;
			}
		}
	
		find3 = message.find("ftp.", curr_pos);
		if (find3 != string::npos && (link_start == string::npos || find3 < link_start)) {
			// ignore if it's not at the beginning or has no whitespace
			if ((find3 - 1) >= 0 && isalnum(message[find3 - 1])) {
				// do nothing
	 		} else if (isspace(message[find3 + 4]) || message[find3 + 4] == '.') {
				// do nothing
			} else {
				link_start = find3;
			}
		}
	}

	return link_count;
}

void ChatWindow::GenerateHyperlinkText(string message, text_run standard, text_run_array **tra) {
	int link_count = CountHyperlinks(message);
	string::size_type pos;
	int link_index = 0;

	// no links?
	if (link_count == 0) {
		*tra = (text_run_array *)malloc(sizeof(text_run_array));

		(*tra)->count = 1;
		(*tra)->runs[0].offset = standard.offset;
		(*tra)->runs[0].font = standard.font;
		(*tra)->runs[0].color = standard.color;

		return;
	}
		
	*tra = (text_run_array *)malloc(sizeof(text_run_array) + (sizeof(text_run) * (link_count * 2 - 1)));
	(*tra)->count = link_count * 2;
		
	string::size_type curr_pos = 0, link_start, link_end;

	// find next link
	link_start = message.find("http://", curr_pos);

	pos = message.find("https://", curr_pos);
	if (pos != string::npos && (link_start == string::npos || pos < link_start)) {
		link_start = pos;
	}	

	pos = message.find("ftp://", curr_pos);
	if (pos != string::npos && (link_start == string::npos || pos < link_start)) {
		link_start = pos;
	}

	pos = message.find("www.", curr_pos);
	if (pos != string::npos && (link_start == string::npos || pos < link_start)) {
		// ignore if it's not at the beginning or has no whitespace
		if ((pos - 1) >= 0 && isalnum(message[pos - 1])) {
			// do nothing
 		} else if (isspace(message[pos + 4]) || message[pos + 4] == '.') {
			// do nothing
		} else {
			link_start = pos;
		}
	}

	pos = message.find("ftp.", curr_pos);
	if (pos != string::npos && (link_start == string::npos || pos < link_start)) {
		// ignore if it's not at the beginning or has no whitespace
		if ((pos - 1) >= 0 && isalnum(message[pos - 1])) {
			// do nothing
 		} else if (isspace(message[pos + 4]) || message[pos + 4] == '.') {
			// do nothing
		} else {
			link_start = pos;
		}
	}
			
	while (link_start != string::npos) {
		// find whitespace or end
		link_end = message.find_first_of(" \t\r\n", link_start);

		if (link_end == string::npos) {
			link_end = message.size() - 1;
		}

		// prune punctuation
		while (link_start < link_end) {
			if (message[link_end] == ',' || message[link_end] == '!' || message[link_end] == '.' || message[link_end] == ')' || message[link_end] == ';' || message[link_end] == ']' || message[link_end] == '>' || message[link_end] == '?' || message[link_end] == '\'' || message[link_end] == '"') {
				--link_end;
			} else {
				break;
			}
		}
		
		// add hyperlink
		if (link_start < link_end) {
			BFont thin(be_plain_font);
			rgb_color light_blue = {0, 0, 192, 255};
			
			(*tra)->runs[link_index].offset = link_start;
			(*tra)->runs[link_index].font = thin;
			(*tra)->runs[link_index].color = light_blue;

			(*tra)->runs[link_index + 1].offset = link_end + 1;
			(*tra)->runs[link_index + 1].font = standard.font;
			(*tra)->runs[link_index + 1].color = standard.color;
		}
		
		curr_pos = link_end + 1;

		if (curr_pos >= message.size()) {
			break;
		}
		
		// find next link
		link_start = message.find("http://", curr_pos);

		pos = message.find("https://", curr_pos);
		if (pos != string::npos && (link_start == string::npos || pos < link_start)) {
			link_start = pos;
		}

		pos = message.find("ftp://", curr_pos);
		if (pos != string::npos && (link_start == string::npos || pos < link_start)) {
			link_start = pos;
		}

		pos = message.find("www.", curr_pos);
		if (pos != string::npos && (link_start == string::npos || pos < link_start)) {
			// ignore if it's not at the beginning or has no whitespace
			if ((pos - 1) >= 0 && isalnum(message[pos - 1])) {
				// do nothing
	 		} else if (isspace(message[pos + 4]) || message[pos + 4] == '.') {
				// do nothing
			} else {
				link_start = pos;
			}
		}

		pos = message.find("ftp.", curr_pos);
		if (pos != string::npos && (link_start == string::npos || pos < link_start)) {
			// ignore if it's not at the beginning or has no whitespace
			if ((pos - 1) >= 0 && isalnum(message[pos - 1])) {
				// do nothing
	 		} else if (isspace(message[pos + 4]) || message[pos + 4] == '.') {
				// do nothing
			} else {
				link_start = pos;
			}
		}

		link_index += 2;
	}
}



void
ChatWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case JAB_CLOSE_TALKS: {
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		
		case JAB_CHAT_SENT:
		{
			const char *messageTextANSI = messageTextView->Text();
			string messageTextSTL = messageTextView->Text();
			BString message = BString(messageTextANSI);
			string s = OurRepresentation().String();
			
			if (_type != GROUP)
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
				
				fprintf(stderr, "username: %s.\n", msg->FindString("username"));
				fprintf(stderr, "show: %s.\n", msg->FindString("show"));
				fprintf(stderr, "status: %s.\n", msg->FindString("status"));
				fprintf(stderr, "role: %s.\n", msg->FindString("role"));
				fprintf(stderr, "affiliation: %s.\n", msg->FindString("affiliation"));
				
				AddGroupChatter(
				
					msg->FindString("username"),
					msg->FindString("show"),
					msg->FindString("status"),
					msg->FindString("role"),
					msg->FindString("affiliation"));
				
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
	jabber->SendUnavailable(BString(_group_room.c_str()), BString("I've been enlightened"));
	TalkManager::Instance()->RemoveWindow(_user->JabberHandle());
	return true;
}

