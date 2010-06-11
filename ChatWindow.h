#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <Window.h>
#include <MenuBar.h>
#include "StatusView.h"
#include "SplitView.h"
#include "JabberProtocol.h"
#include "UserID.h"
#include <cstdio>
#include <deque>
#include <string>
#include <storage/FilePanel.h>

class ChatWindow : public BWindow
{
public:
enum        talk_type {MESSAGE, CHAT, GROUP};
enum        user_type {MAIN_RECIPIENT, LOCAL, OTHER};
			
						ChatWindow(talk_type type, UserID *user, std::string group_room,
				std::string group_username);
			void		MessageReceived(BMessage *msg);
			bool		QuitRequested(void);
			void		FrameResized(float width, float height);
			void		AddToTalk(string username, string message, user_type type);
			void		NewMessage(string new_message);
			void		NewMessage(string username, string new_message);
			BString		OurRepresentation();
			string		GetGroupUsername();
			string		GetGroupRoom();
	const	UserID		*GetUserID();
			void		SetThreadID(string id);

			JabberProtocol *jabber;
						
private:
	//std::deque<std::string>  _chat_history;
	std::string              _chat_buffer;
	int                 _chat_index;
	
	BFilePanel         *_fp;
	bool                _am_logging;
	FILE               *_log;
	BString				originalWindowTitle;
			UserID      			*_user;
			std::string 			_group_room;
			std::string            _group_username;
			UserID::online_status  _current_status;
			talk_type              _type;
			std::string            _thread;
	
			BMenuBar	*menu;
			StatusView	*statusView;
			SplitPane	*chatView;
			BView		*messageView;
			BView		*historyView;
			BView		*mainView;
			BScrollView	*messageScroller;
			BScrollView	*historyScroller;
			BTextView	*historyTextView;
			BTextView	*messageTextView;

};

#endif
