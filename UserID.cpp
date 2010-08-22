//////////////////////////////////////////////////
// Blabber [UserID.cpp]
//////////////////////////////////////////////////

#include <algorithm>
#include <string>
#include "Agent.h"
#include "AgentList.h"
#include "UserID.h"

UserID::UserID(std::string handle)
{
	SetHandle(handle);
	SetUsertype(UserID::JABBER);
	SetOnlineStatus(UserID::OFFLINE);
	SetSubscriptionStatus("none");
		
	//SetExactOnlineStatus("chat");
	//SetMoreExactOnlineStatus("");
}

UserID::UserID(const UserID &copied_userid)
{
	SetHandle(copied_userid.Handle());
	SetFriendlyName(copied_userid.FriendlyName());
	SetSubscriptionStatus(copied_userid.SubscriptionStatus());
	SetOnlineStatus(copied_userid.OnlineStatus());
	SetUsertype(copied_userid.UserType());
	
	//SetExactOnlineStatus(copied_userid.ExactOnlineStatus());
	//SetMoreExactOnlineStatus(copied_userid.MoreExactOnlineStatus());
}

UserID::~UserID()
{
}

UserID &UserID::operator=(const UserID &rhs)
{
	SetHandle(rhs.Handle());
	SetFriendlyName(rhs.FriendlyName());
	SetSubscriptionStatus(rhs.SubscriptionStatus());
	SetOnlineStatus(rhs.OnlineStatus());
	SetUsertype(rhs.UserType());

	//SetExactOnlineStatus(rhs.ExactOnlineStatus());
	//SetMoreExactOnlineStatus(rhs.MoreExactOnlineStatus());

	return *this;
}

const UserID::user_type UserID::UserType() const
{
	return _user_type;
}

const std::string UserID::Handle() const
{
	return _handle;
}

const std::string UserID::FriendlyName() const
{
	return _friendly_name;
}

const UserID::online_status UserID::OnlineStatus() const
{
	return _status;
}

const std::string UserID::ExactOnlineStatus() const
{
	if (OnlineStatus() == ONLINE) {
		// exact status only applies for users online
		return _exact_status;
	} else {
		// user is offline, exact status is invalid
		return "";
	}
}

const std::string UserID::MoreExactOnlineStatus() const {
	if (OnlineStatus() == ONLINE) {
		// more exact status only applies for users online
		return _more_exact_status;
	} else {
		// user is offline, more exact status is invalid
		return "";
	}
}

const std::string UserID::SubscriptionStatus() const
{
	return _subscription_status;
}

bool UserID::HaveSubscriptionTo() const
{
	return (SubscriptionStatus() == "to" || SubscriptionStatus() == "both");
}

bool UserID::IsUser() const
{
	return (UserType() == JABBER || UserType() == CONFERENCE);
}

void UserID::SetUsertype(user_type type)
{
	_user_type = type;
}

const std::string UserID::JabberHandle() const
{
	return JabberUsername() + '@'+ JabberServer();
}

const std::string UserID::JabberCompleteHandle() const
{
	std::string complete_handle;

	// get handle
	std::string handle = JabberHandle();

	if (!handle.empty()) {
		complete_handle = handle;

		std::string resource = JabberResource();
		if (!resource.empty()) {
			complete_handle += "/" + resource;
		}
	}

	return complete_handle;
}

const std::string UserID::JabberUsername() const { return _jabber_username; }

const std::string UserID::JabberServer() const { return _jabber_server; }

const std::string UserID::JabberResource() const { return _jabber_resource; }

const std::string UserID::TransportID() const { return _transport_id; }

const std::string UserID::TransportUsername() const { return _transport_username; }

const std::string UserID::TransportPassword() const { return _transport_password; }

std::string UserID::WhyNotValidJabberHandle()
{
	if (UserType() == JABBER) {
		return "";
	}

	if (_jabber_username.size() == 0 || _jabber_server.size() == 0) {
		return "Jabber ID must be of the form username@server[/resource].";
	}

	// verify length
	if (_jabber_username.size() > 255) {
		return "Jabber ID username part must not be longer than 255 characters.";
	}

	// verify ASCII charactership of entire login
	for (uint i=0; i<_handle.size(); ++i) {
		if (int(_handle[i]) < 33) {
			return "Jabber ID must not contain unprintable characters.";
		}

		if (isspace(int(_handle[i]))) {
			return "Jabber ID must not contain whitespace.";
		}

		if (iscntrl(int(_handle[i]))) {
			return "Jabber ID must not contain control characters.";
		}
	}

	// verify ASCII charactership of abbreviated username
	if (_jabber_username.find_first_of(":@<>'\"&") != std::string::npos) {
		return "Jabber ID username part must not contain any of the following characters in the handle: @<>:'\"&";
	}

	// no errors found
	return "";
}

void UserID::SetHandle(std::string handle)
{
	// initialize values
	_handle = handle;

	// process based on username structure
	_ProcessHandle();
}

void UserID::SetFriendlyName(std::string friendly_name) { _friendly_name = friendly_name; }

void UserID::SetOnlineStatus(online_status status)
{
	_status = status;
}

void UserID::SetExactOnlineStatus(std::string exact_status)
{
	_exact_status = exact_status;
	SetMoreExactOnlineStatus("");
}

void UserID::SetMoreExactOnlineStatus(std::string more_exact_status)
{
	_more_exact_status = more_exact_status;
}

void UserID::SetSubscriptionStatus(std::string status) {
	// only set legal status
	if (status == "none" || status == "to" || status == "from" || status == "both") {
		_subscription_status = status;
	} else {
		return;
	}

	// changing subscription may change status
	if (!HaveSubscriptionTo()) {
		SetOnlineStatus(UserID::UNKNOWN);
		if (UserType() == CONFERENCE)
			_status = CONF_STATUS;
	} else {
		if (OnlineStatus() == UserID::UNKNOWN) {
			SetOnlineStatus(UserID::OFFLINE);
		}
	}
}

void UserID::_ProcessHandle()
{
	////////// Split into Jabber pieces
	{
		uint squigly_pos, slash_pos;

		// reset split values
		_jabber_username  = "";
		_jabber_server    = "";
		_jabber_resource  = "";

		// extract abbreviated username (text between start of username and @)
		squigly_pos = _handle.find("@");

		if (squigly_pos != std::string::npos && squigly_pos != 0) {
			// extract the abbreviated username
			_jabber_username = _handle.substr(0, squigly_pos);
		} else {
			return;
		}

		// is there still reason to go on (is there more text)?
		if ((_handle.size() - 1) != squigly_pos) {
			// extract server name (text between @ and /)
			slash_pos = _handle.find("/", squigly_pos);

			if (slash_pos == std::string::npos) {
				// all the rest is the server name (there is no resource)
				_jabber_server = _handle.substr(squigly_pos + 1, _handle.size() - squigly_pos);
			} else {
				// extract server name
				_jabber_server = _handle.substr(squigly_pos + 1, slash_pos - squigly_pos - 1);

				// extract resource name
				_jabber_resource = _handle.substr(slash_pos + 1, _handle.size() - slash_pos - 1);
			}
		}
	}

}
