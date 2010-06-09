//////////////////////////////////////////////////
// Blabber [TalkManager.h]
//     Handles the simultaneous talk sessions
//     going on.
//////////////////////////////////////////////////

#ifndef TALK_MANAGER_H
#define TALK_MANAGER_H

#include <map>
#include <string>

#include "GenericFunctions.h"
#include "ChatWindow.h"
#include "XMLEntity.h"

class TalkManager {
public:
	typedef map<string, ChatWindow*>                   TalkMap;
	typedef map<string, ChatWindow*>::iterator         TalkIter;
	typedef map<string, ChatWindow*>::const_iterator   ConstTalkIter;

	enum     rotation   {ROTATE_FORWARD, ROTATE_BACKWARD};
		
public:
	static TalkManager  *Instance();
      	                ~TalkManager();

	ChatWindow          *CreateTalkSession(
							ChatWindow::talk_type type,
							UserID *user,
							string group_room, string group_username);
				
	void                 ProcessMessageData(XMLEntity *entity);	

	std::string          IsExistingWindowToUser(string username);
	std::string          IsExistingWindowToGroup(string group_room);
	void                 UpdateWindowTitles(const UserID *user);
	void                 RemoveWindow(string thread_id);

	void                 RotateToNextWindow(ChatWindow *current, rotation direction);
	
	void                 Reset();
	JabberProtocol		 *jabber;
	
protected:
 	                     TalkManager();

private:
	static TalkManager *_instance;
	
	TalkMap             _talk_map;
};

#endif

