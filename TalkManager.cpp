//////////////////////////////////////////////////
// Blabber [TalkManager.cpp]
//////////////////////////////////////////////////

#include <interface/Window.h>
#include "Settings.h"
#include "ChatWindow.h"
#include "JRoster.h"
#include "MessageRepeater.h"
#include "Messages.h"
#include "ModalAlertFactory.h"
#include "TalkManager.h"

TalkManager *TalkManager::_instance = NULL;

TalkManager *TalkManager::Instance() {
	if (_instance == NULL) {
		_instance = new TalkManager();
	}

	return _instance;
}

TalkManager::TalkManager() { }

TalkManager::~TalkManager() {
	for (TalkIter i = _talk_map.begin(); i != _talk_map.end(); ++i) {
		i->second->PostMessage(B_QUIT_REQUESTED);
	}

	_instance = NULL;
}

ChatWindow *TalkManager::CreateTalkSession(ChatWindow::talk_type type, UserID *user, string group_room, string group_username)
{
	ChatWindow *window = NULL;
	
	if (IsExistingWindowToUser(user->JabberHandle()).size())
	{
		window = _talk_map[IsExistingWindowToUser(user->JabberHandle())];
		window->Lock();
		window->Activate();
		window->Unlock();
	} 
	else
	{
		window = new ChatWindow(type, user, group_room, group_username);
		window->jabber = jabber;
		_talk_map[user->JabberHandle()] = window;
		
		if (type == ChatWindow::GROUP)
		{
			jabber->SendGroupPresence(group_room, group_username);
		}
	}
	
	return window;
}

void TalkManager::ProcessMessageData(XMLEntity *entity)
{
	ChatWindow::talk_type type;
	string                thread_id;
	string                sender;
	string				  receiver;
	ChatWindow           *window = NULL;

	string                group_room;
	string                group_server;
	string                group_identity;
	string                group_username;
	
	// must be content to continue
	if (!entity->Child("body") || !entity->Child("body")->Data())
	{
		return;
	}

	// must be sender to continue
	if (!entity->Attribute("from"))
	{
		return;
	}

	// determine message type
	if (!entity->Attribute("type"))
	{
		if (entity->Child("x", "xmlns", "jabber:x:oob"))
			type = ChatWindow::GROUP;
	}
	else if (!strcasecmp(entity->Attribute("type"), "normal"))
	{
		if (BlabberSettings::Instance()->Tag("convert-messages-to-chat")) 
			type = ChatWindow::CHAT;
		else
			type = ChatWindow::MESSAGE;
	}
	else if (!strcasecmp(entity->Attribute("type"), "chat"))
	{
		type = ChatWindow::CHAT;
	}
	else if (!strcasecmp(entity->Attribute("type"), "groupchat"))
	{
		type = ChatWindow::GROUP;
	}
	else if (!strcasecmp(entity->Attribute("type"), "error"))
	{
		char buffer[2048];
		
		if (entity->Child("error")) {
			const char *text = entity->Child("error")->Child("text") ? entity->Child("error")->Child("text")->Data() : "Unknown";
			sprintf(buffer, "An error occurred when you tried sending a message to %s.  The reason is as follows:\n\n%s", entity->Attribute("from"), text);
			ModalAlertFactory::Alert(buffer, "Oh, well.", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
		}
		
		return;
	}
	else
	{
		return;
	}
	
	// configure routing information
	sender = entity->Attribute("from");
	receiver = entity->Attribute("to");
	group_username = UserID(sender).JabberResource();
	group_room = UserID(sender).JabberUsername();

	// find window or create new
	if (type == ChatWindow::CHAT || type == ChatWindow::GROUP)
	{
		if (IsExistingWindowToUser(UserID(sender).JabberHandle()) != "")
		{
			window = _talk_map[IsExistingWindowToUser(UserID(sender).JabberHandle())];
			if (window != NULL)
				fprintf(stderr, "Redirected to Existed Window: %s.\n",window->GetUserID()->JabberHandle().c_str());
		}
		else
		{
			JRoster::Instance()->Lock();
			
			UserID *user = JRoster::Instance()->FindUser(new UserID(sender));
			
			if (!user)
			{
				fprintf(stderr, "Not found incoming message user in roster.\n");
			}

			JRoster::Instance()->Unlock();
			
			if (type==ChatWindow::CHAT)
			{
				window = CreateTalkSession(type, user, "", "");
				window->jabber = jabber;
			}
		}
	}
	
	// submit the chat to window
	if (window)
	{
		window->Lock();
		
		if (type == ChatWindow::CHAT)
		{
			window->NewMessage(entity->Child("body")->Data());
		}
		else if (type == ChatWindow::GROUP)
		{
			// Accept exectly our JID in destinations 
			if (receiver != UserID(jabber->jid.String()).JabberCompleteHandle())
			{
				window->Unlock();
				return;
			}
							
			if (group_username == "")
				window->NewMessage(group_room, entity->Child("body")->Data()); // channel messages
			else
				window->NewMessage(group_username, entity->Child("body")->Data()); // user messages
		}
		
		window->Unlock();
	}
}

string TalkManager::IsExistingWindowToUser(string username) {
	// check handles (with resource)
	int j = 0;
	for (TalkIter i = _talk_map.begin(); i != _talk_map.end(); i++) {
		//fprintf(stderr, "iter %i: %s\n", j++, (*i).second->GetUserID()->JabberHandle().c_str());
		if ((*i).second->GetUserID()->JabberHandle() == UserID(username).JabberHandle()) {
			return UserID(username).JabberHandle();
		}
	}

	// no matches
	return "";
}

string TalkManager::IsExistingWindowToGroup(string group_room) {
	// check names
	for (TalkIter i = _talk_map.begin(); i != _talk_map.end(); ++i) {
		if ((*i).second->GetGroupRoom() == group_room) {
			return (*i).first;
		}
	}

	// no matches
	return "";
}

void TalkManager::UpdateWindowTitles(const UserID *user) {
	// check handles (without resource)
	for (TalkIter i = _talk_map.begin(); i != _talk_map.end(); ++i) {
		if ((*i).second->GetUserID()->JabberHandle() == user->JabberHandle()) {
			(*i).second->SetTitle(user->FriendlyName().c_str());
		}
	}
}

void TalkManager::RemoveWindow(string username) {
	if (_talk_map.count(username) > 0) {
		_talk_map.erase(username);
	}
}

void TalkManager::RotateToNextWindow(ChatWindow *current, rotation direction) {
	// no chat windows
	if (_talk_map.size() == 0) {
		return;
	}

	// from chat windows
	if (_talk_map.size() == 1 && current != NULL) {
		return;
	}

	// remember first and last, we may need them later
	ChatWindow *first = (*_talk_map.begin()).second;
	ChatWindow *last  = (*_talk_map.rbegin()).second;
		
	// from non-chat windows
	if (current == NULL) {
		if (direction == ROTATE_FORWARD) {
			first->Activate();
		} else {
			last->Activate();
		}
		
		return;
	}
	
	// iterate and find the current window
	ChatWindow *previous = NULL;
	for (TalkIter i = _talk_map.begin(); i != _talk_map.end(); ++i) {
		if ((*i).second == current) {
			// we found our window, now check bordering windows
			if (direction == ROTATE_FORWARD) {
				if (++i != _talk_map.end()) {
					(*i).second->Activate();
				} else {
					first->Activate();
				}
			} else {
				if (previous) {
					previous->Activate();
				} else {
					last->Activate();
				}
			}

			break;
		} else {
			previous = (*i).second;
		}
	}
}

void TalkManager::Reset() {
	MessageRepeater::Instance()->PostMessage(JAB_CLOSE_TALKS);
	//_talk_map.clear();
}
