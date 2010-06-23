//////////////////////////////////////////////////
// Blabber [JRoster.cpp]
//////////////////////////////////////////////////

#include "MessageRepeater.h"
#include <kernel/OS.h>
#include "Messages.h"
#include "Agent.h"
#include "AgentList.h"
#include "JRoster.h"
#include <string>
#include <string.h>

JRoster *JRoster::_instance = NULL;

JRoster *
JRoster::Instance()
{
	if (_instance == NULL) {
		_instance = new JRoster();
	}
	return _instance;	
}

JRoster::~JRoster()
{
	_instance = NULL;
	// destroy semaphore
	delete_sem(_roster_lock);
}
	    
void
JRoster::AddRosterUser(UserID *roster_user)
{
	_roster->push_back(roster_user);
	// refresh all roster views
	RefreshRoster();
}

void
JRoster::AddNewUser(UserID *new_user)
{
	_roster->push_back(new_user);

	// communicate the new user to the server
	//JabberSpeak::Instance()->AddToRoster(new_user);

	// refresh all roster views
	RefreshRoster();
}

void
JRoster::RemoveUser(const UserID *removed_user)
{
	for (RosterIter i = _roster->begin(); i != _roster->end(); ++i) {
		if (*i == removed_user) {
			_roster->erase(i);

			// for transports
			if (removed_user->UserType() == UserID::TRANSPORT) {
				Agent *agent = AgentList::Instance()->GetAgentByID(removed_user->TransportID());

				if (agent) {
					agent->SetRegisteredFlag(false);
				}
			}
			
			// goodbye memory
			delete removed_user;

			break;	
		}
	}
}

void
JRoster::RemoveAllUsers()
{
	// BUGBUG need more elegant way of deleting all users (check STL guide)
	while(_roster->begin() != _roster->end()) {
		RosterIter i = _roster->begin();
		UserID *user = *i;

		_roster->erase(i);
		delete user;
	}
}
	
UserID *
JRoster::FindUser(search_method search_type, std::string name)
{
	if (search_type == JRoster::FRIENDLY_NAME) {
		for (RosterIter i = _roster->begin(); i != _roster->end(); ++i) {
			if (!strcasecmp(name.c_str(), (*i)->FriendlyName().c_str())) {
				return (*i);
			}
		}
	}
	
	if (search_type == JRoster::HANDLE) {
		for (RosterIter i = _roster->begin(); i != _roster->end(); ++i) {
			if (!strcasecmp(name.c_str(), (*i)->JabberHandle().c_str())) {
				return (*i);
			}
		}
	}

	if (search_type == JRoster::COMPLETE_HANDLE) {
		for (RosterIter i = _roster->begin(); i != _roster->end(); ++i) {
			if (!strcasecmp(name.c_str(), (*i)->JabberCompleteHandle().c_str())) {
				return (*i);
			}
		}
	}

	if (search_type == JRoster::TRANSPORT_ID) {
		for (RosterIter i = _roster->begin(); i != _roster->end(); ++i) {
			if (!strcasecmp(name.c_str(), (*i)->TransportID().c_str())) {
				return (*i);
			}
		}
	}

	return NULL;
}

bool
JRoster::ExistingUserObject(const UserID *comparing_user)
{
	for (RosterIter i = _roster->begin(); i != _roster->end(); ++i) {
		if ((*i) == comparing_user) {
			return true;
		}
	}
	
	return false;
}

UserID *
JRoster::FindUser(const UserID *comparing_user)
{
	return FindUser(JRoster::HANDLE, comparing_user->JabberHandle());
}

void
JRoster::SetUserStatus(std::string username, UserID::online_status status)
{
	UserID *user = const_cast<UserID *>(FindUser(COMPLETE_HANDLE, username));
	
	if (user != NULL) {
		user->SetOnlineStatus(status);
	}

	// refresh all roster views
	RefreshRoster();
}

const
UserID::online_status JRoster::UserStatus(std::string username)
{
	UserID *user = FindUser(COMPLETE_HANDLE, username);
	if (user != NULL)
	{
		return user->OnlineStatus();
	}
}

JRoster::ConstRosterIter
JRoster::BeginIterator()
{
	return _roster->begin();
}

JRoster::ConstRosterIter
JRoster::EndIterator()
{
	return _roster->end();
}

void
JRoster::RefreshRoster()
{
	// update everyone to the change
	MessageRepeater::Instance()->PostMessage(BLAB_UPDATE_ROSTER);
	//MessageRepeater::Instance()->PostMessage(TRANSPORT_UPDATE);
}

void
JRoster::Lock()
{
	acquire_sem(_roster_lock);
}

void
JRoster::Unlock()
{
	release_sem(_roster_lock);
}

JRoster::JRoster()
{
	_roster = new RosterList;
	_roster_lock = create_sem(1, "roster sempahore");
}
