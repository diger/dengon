/*
 * Copyright 2010 Maxim Sokhatsky <maxim@synrc.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
 
#include "JabberProtocol.h"
#include "ChatWindow.h"
#include "GenericFunctions.h"
#include "ModalAlertFactory.h"
#include "MainWindow.h"
#include "MessageRepeater.h"
#include "TalkManager.h"
#include "Messages.h"
#include "Base64.h"
#include <OS.h>
#include <sys/utsname.h>
#include <Roster.h>
#include <unistd.h>

#include "version.h"

#define DEBUG 

static int32 SessionDispatcher(void *args);
static int zeroReceived = 0;
static int separatorReceived = 0;

JabberProtocol::JabberProtocol()
{
	logged = create_sem(1, "logged");
	read_queue = create_sem(1, "read_queue");
	xml_reader_lock = create_sem(1, "xml_reader");
	reciever_thread = spawn_thread(SessionDispatcher, "xmpp_receiver", B_LOW_PRIORITY, this);
	socketAdapter = new SSLAdapter();
}

JabberProtocol::~JabberProtocol()
{
}

void
JabberProtocol::SetConnection(BString h, int p, bool s)
{
	host = h;
	port = p;
	secure = s;
}

void
JabberProtocol::SetCredentials(BString u, BString d, BString p)
{
	domain = d;
	user = u;
	pass = p;
}

void
JabberProtocol::LockXMLReader() {
	acquire_sem(xml_reader_lock);
}

void
JabberProtocol::UnlockXMLReader() {
	release_sem(xml_reader_lock);
}

void 
JabberProtocol::LogOn() 
{
	zeroReceived = 0;
	separatorReceived = 0;

	if (BeginSession())
	{
		Authorize();
		
		acquire_sem(logged);
		resume_thread(reciever_thread);
	}
}

void
JabberProtocol::SetStatus(BString status, BString message)
{
	acquire_sem(logged);
	release_sem(logged);
	
	BString xml;
	xml << "<presence from='";
	xml = xml.Append(jid);
	xml << "'><show>";
	xml = xml.Append(status);
	xml << "</show><status>";
	xml = xml.Append(message);
	xml << "</status></presence>";
	
	socketAdapter->SendData(xml);
	
}

void
JabberProtocol::OnStartTag(XMLEntity *entity)
{
}

void
JabberProtocol::OnEndTag(XMLEntity *entity)
{
	OnTag(entity);
}

void
JabberProtocol::OnEndEntity(XMLEntity *entity)
{
	
}

void
JabberProtocol::OnTag(XMLEntity *entity)
{
	char buffer[4096]; // general buffer space
	static int seen_streams = 0;
	string iq_id;      // used for IQ tags

	if (entity->IsCompleted() && !strcasecmp(entity->Name(), "iq"))
	{
		if (entity->Attribute("id")) {
			iq_id = entity->Attribute("id");
		//}
		
		// handle roster retrival
		if (!strcasecmp(entity->Attribute("type"), "result") &&
			!strcasecmp(entity->Attribute("id"), "roster_2"))
		{
			ParseRosterList(entity);
		}
		
		// handle session retrival
		if (!strcasecmp(entity->Attribute("type"), "result") &&
			!strcasecmp(entity->Attribute("id"), "sess_1"))
		{
			release_sem(logged);
		
			mainWindow->Lock();
			mainWindow->PostMessage(JAB_LOGGED_IN);
			mainWindow->Unlock();
		}
		
		// handle binding retrival
		if (!strcasecmp(entity->Attribute("type"), "result") &&
			!strcasecmp(entity->Attribute("id"), "bind_0"))
		{
			jid = BString(entity->Child("bind")->Child("jid")->Data());

#ifdef DEBUG			
			fprintf(stderr, "JID: %s.\n", jid.String());
#endif

			Session();
		}
		
		}
		
		if (!strcasecmp(entity->Attribute("type"), "get"))
		{
			string iq_from;   
			if (entity->Attribute("from")) {
				iq_from = entity->Attribute("from");
			}
			
			// handle version request
			XMLEntity *query = entity->Child("query");
			if (query && query->Attribute("xmlns")) {
				if (!strcasecmp(query->Attribute("xmlns"), "jabber:iq:version")) {
					ProcessVersionRequest(iq_id, iq_from);
				}
			}
			
			// handle version request
			query = entity->Child("ping");
			if (query && query->Attribute("xmlns")) {
				if (!strcasecmp(query->Attribute("xmlns"), "urn:xmpp:ping"))
				{
					Pong(BString(entity->Attribute("id")), BString(entity->Attribute("from")));
				}
			}
		}
		
		if (!strcasecmp(entity->Attribute("type"), "set"))
		{
			XMLEntity *query = entity->Child("query");
			if (query && query->Attribute("xmlns")) {
				if (!strcasecmp(query->Attribute("xmlns"), "jabber:iq:roster"))
				{
					XMLEntity *removed_user = query->Child("item");
					string jid = removed_user->Attribute("jid");
					
					if (!strcasecmp(removed_user->Attribute("subscription"), "remove"))
					{
						JRoster::Instance()->RemoveUser(
							JRoster::Instance()->FindUser(JRoster::HANDLE, jid));
						
						mainWindow->Lock();
						mainWindow->PostMessage(BLAB_UPDATE_ROSTER);
						mainWindow->Unlock();
					}
				}
			}
		}
	}
	
	// handle authorization success
	if (entity->IsCompleted() && !strcasecmp(entity->Name(), "success"))
	{
		InitSession();
		Bind();
	}
	
	// handle presence messages
	if (entity->IsCompleted() && !strcasecmp(entity->Name(), "presence"))
	{
		ProcessPresence(entity);
	}
	
	// handle stream error
	if (entity->IsCompleted() && !strcasecmp(entity->Name(), "stream:error")) {
		sprintf(buffer, "An stream error has occurred.");
		ModalAlertFactory::Alert(buffer, "Sorry", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT); 
		
		Disconnect();
	}
	
	// handle failures
	if (entity->IsCompleted() && !strcasecmp(entity->Name(), "failure")) {
		if (entity->Child("not-authorized") != NULL)
			sprintf(buffer, "Not authorized failure.");
		else if (entity->Child("invalid-mechanism") != NULL)
			sprintf(buffer, "Invalid mechanism failure.");
		else if (entity->Child("invalid-authzid") != NULL)
			sprintf(buffer, "Invalid authorization Id.");
		else
			sprintf(buffer, "An failure occured.");
			
		ModalAlertFactory::Alert(buffer, "Sorry", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT); 
		
		Disconnect();
	}
	
	// handle connection
	if (!entity->IsCompleted() && !strcasecmp(entity->Name(), "stream:stream"))
	{
		
	}

	// handle disconnection
	if (entity->IsCompleted() && !strcasecmp(entity->Name(), "stream:stream")) {
		++seen_streams;
		
		if (seen_streams % 2 == 1) {
			
			Disconnect();
		}
	}
	
	// handle incoming messages
	if (entity->IsCompleted() && !strcasecmp(entity->Name(), "message"))
	{
		TalkManager::Instance()->ProcessMessageData(entity);
	}
}

void
JabberProtocol::Pong(BString id, BString from)
{
	BString xml = "<iq from='";
	xml = xml.Append(jid);
	xml << "' id='";
	xml = xml.Append(id);
	xml << "' to='";
	xml = xml.Append(from);
	xml << "' type='result'/>";

#ifdef DEBUG	
	fprintf(stderr, "Pong reply to %s.\n", from.String());
#endif
	
	socketAdapter->SendData(xml);
}

void
JabberProtocol::SendUnavailable(BString to, BString status)
{
	BString xml = "<presence to='";
	xml = xml.Append(to);
	xml << "' type='unavailable'><status>";
	xml = xml.Append(status);
	xml << "</status></presence>";
	
	socketAdapter->SendData(xml);
}

void
JabberProtocol::ProcessVersionRequest(string req_id, string req_from)
{
	XMLEntity   *entity_iq, *entity_query;
	char **atts_iq    = CreateAttributeMemory(6);
	char **atts_query = CreateAttributeMemory(2);

	// assemble attributes;
	strcpy(atts_iq[0], "id");
	strcpy(atts_iq[1], req_id.c_str());

	strcpy(atts_iq[2], "to");
	strcpy(atts_iq[3], req_from.c_str());

	strcpy(atts_iq[4], "type");
	strcpy(atts_iq[5], "result");

	strcpy(atts_query[0], "xmlns");
	strcpy(atts_query[1], "jabber:iq:version");

	// construct XML tagset
	entity_iq    = new XMLEntity("iq", (const char **)atts_iq);
	entity_query = new XMLEntity("query", (const char **)atts_query);

	entity_iq->AddChild(entity_query);
	
	entity_query->AddChild("name", NULL, "Dengon");
	entity_query->AddChild("version", NULL, "1.0 (rev: "DENGON_SVNVERSION")");

	string strVersion("Haiku");
	
	string os_info;
	utsname uname_info;
	if (uname(&uname_info) == 0) {
		os_info = uname_info.sysname;
		long revision = 0;
		if (sscanf(uname_info.version, "r%ld", &revision) == 1) {
			char version[16];
			snprintf(version, sizeof(version), "%ld", revision);
			os_info += " (rev: ";
			os_info += version;
			os_info += ")";
		}
	}

	entity_query->AddChild("os", NULL, os_info.c_str());

	// send XML command
	char *str = entity_iq->ToString();
	socketAdapter->SendData(BString(str));
	free(str);

	DestroyAttributeMemory(atts_iq, 6);
	DestroyAttributeMemory(atts_query, 2);
	
	delete entity_iq;
}

void
JabberProtocol::SendGroupPresence(string _group_room, string _group_username)
{
	XMLEntity             *entity_presence;
	char **atts_presence = CreateAttributeMemory(2);

	// assemble group ID
	string group_presence = _group_room + "/" + _group_username;	
	
	// assemble attributes;
	strcpy(atts_presence[0], "to");
	strcpy(atts_presence[1], group_presence.c_str());

	// construct XML tagset
	entity_presence = new XMLEntity("presence", (const char **)atts_presence);
	
	// send XML command
	char *str = entity_presence->ToString();
	socketAdapter->SendData(BString(str));
	free(str);

	DestroyAttributeMemory(atts_presence, 2);
	
	delete entity_presence;
}

void
JabberProtocol::ProcessPresence(XMLEntity *entity)
{
	JRoster *roster = JRoster::Instance();

	int num_matches = 0;

	// verify we have a username
	if (entity->Attribute("from"))
	{
		roster->Lock();

		// circumvent groupchat presences
		string room, server, user;
		
		if (entity->Child("x", "xmlns", "http://jabber.org/protocol/muc#user"))
		{
			UserID from = UserID(entity->Attribute("from"));
			room = from.JabberUsername();
			server = from.JabberServer();
			user = from.JabberResource();
			fprintf(stderr, "Group Presence in room %s from user %s.\n", 
				(room + '@' + server).c_str(), user.c_str());
						
			BMessage msg;
			msg.AddString("room", (room + '@' + server).c_str());
			msg.AddString("server", server.c_str());
			msg.AddString("username", user.c_str());
			
			if (!entity->Attribute("type") || !strcasecmp(entity->Attribute("type"), "available"))
			{
				if (entity->Child("show") && entity->Child("show")->Data())
				{
					msg.AddString("show", entity->Child("show")->Data());
				} else
					msg.AddString("show", "online");

				if (entity->Child("status") && entity->Child("status")->Data())
				{
					msg.AddString("status", entity->Child("status")->Data());
				} else
					msg.AddString("status", "");
					
				msg.AddString("role", "admin");
				msg.AddString("affiliation", "none");
		
				msg.what = JAB_GROUP_CHATTER_ONLINE;
			}
			else if (!strcasecmp(entity->Attribute("type"), "unavailable"))
			{
				msg.what = JAB_GROUP_CHATTER_OFFLINE;
			}

			MessageRepeater::Instance()->PostMessage(&msg);

			roster->Unlock();
			return;
		}		

		for (JRoster::ConstRosterIter i = roster->BeginIterator(); i != roster->EndIterator(); ++i)
		{
			UserID *user = NULL;

			if ((*i)->IsUser() &&
				!strcasecmp(UserID(entity->Attribute("from")).JabberHandle().c_str(), (*i)->JabberHandle().c_str()))
			{
				++num_matches;
				user = *i;
				ProcessUserPresence(user, entity);
			}
			else if ((*i)->UserType() == UserID::TRANSPORT &&
					!strcasecmp(UserID(entity->Attribute("from")).TransportID().c_str(), (*i)->TransportID().c_str()))
			{
				++num_matches;
				user = *i;
				ProcessUserPresence(user, entity);
			}
		}
		
		if (num_matches == 0)
		{
			UserID user(entity->Attribute("from"));
			ProcessUserPresence(&user, entity);
		}
			
		roster->Unlock();

		mainWindow->PostMessage(BLAB_UPDATE_ROSTER);			
	}
}

char **
JabberProtocol::CreateAttributeMemory(int num_items)
{
	char **atts;
	
	atts = (char **)malloc((num_items + 2) * sizeof(char *));
	for (int i=0; i<num_items; ++i)
		atts[i] = (char *)malloc(96 * sizeof(char));
	
	atts[num_items] = NULL;
	atts[num_items+1] = NULL;
	
	return atts;
}

void
JabberProtocol::Disconnect()
{
	Reset();
	JRoster::Instance()->Lock();
	JRoster::Instance()->RemoveAllUsers();
	JRoster::Instance()->Unlock();
	
	mainWindow->Lock();
	mainWindow->PostMessage(BLAB_UPDATE_ROSTER);
	mainWindow->ShowLogin();
	mainWindow->Unlock();
	
	BString xml = "</stream:stream>";
	socketAdapter->SendData(xml);
	socketAdapter->Close();
	
	release_sem(logged);
	suspend_thread(reciever_thread);
}

void
JabberProtocol::DestroyAttributeMemory(char **atts, int num_items)
{
	for (int i=0; i<(num_items + 2); ++i) {
		free(atts[i]);
	}
	
	free(atts);
}

void
JabberProtocol::AcceptPresence(string username) {
	XMLEntity *entity;
	
	char **atts = CreateAttributeMemory(4);
	
	// assemble attributes
	strcpy(atts[0], "to");
	strcpy(atts[1], username.c_str());
	strcpy(atts[2], "type");
	strcpy(atts[3], "subscribed");

	entity = new XMLEntity("presence", (const char **)atts);

	// send XML command
	char *str = entity->ToString();
	socketAdapter->SendData(BString(str));
	free(str);
	
	DestroyAttributeMemory(atts, 4);
	delete entity;
}

void
JabberProtocol::RejectPresence(string username)
{
	XMLEntity *entity;
	
	char **atts = CreateAttributeMemory(4);
	
	// assemble attributes
	strcpy(atts[0], "to");
	strcpy(atts[1], username.c_str());
	strcpy(atts[2], "type");
	strcpy(atts[3], "unsubscribed");

	entity = new XMLEntity("presence", (const char **)atts);

	// send XML command
	char *str = entity->ToString();
	socketAdapter->SendData(BString(str));
	free(str);
	
	DestroyAttributeMemory(atts, 4);
	delete entity;
}

string
JabberProtocol::GenerateUniqueID()
{
	static long counter = 0;
	pid_t pid = getpid();
	time_t secs = time(NULL);
	++counter;
	char buffer[100];
	sprintf(buffer, "%lu:%lu:%lu", pid, secs, counter);
	return string(buffer);
}


void
JabberProtocol::SendSubscriptionRequest(string username)
{
	XMLEntity *entity;
	
	char **atts = CreateAttributeMemory(6);
	
	// assemble attributes
	strcpy(atts[0], "to");
	strcpy(atts[1], username.c_str());
	strcpy(atts[2], "type");
	strcpy(atts[3], "subscribe");
	strcpy(atts[4], "id");
	strcpy(atts[5], GenerateUniqueID().c_str());

	entity = new XMLEntity("presence", (const char **)atts);

	// log command
	//_iq_map[atts[5]] = LOGIN;
	
	// send XML command
	char *str = entity->ToString();
	socketAdapter->SendData(BString(str));
	free(str);
	
	DestroyAttributeMemory(atts, 6);
	delete entity;
}

void
JabberProtocol::SendUnsubscriptionRequest(string username)
{
	XMLEntity *entity;
	
	char **atts = CreateAttributeMemory(6);
	
	// assemble attributes
	strcpy(atts[0], "to");
	strcpy(atts[1], username.c_str());
	strcpy(atts[2], "type");
	strcpy(atts[3], "unsubscribe");
	strcpy(atts[4], "id");
	strcpy(atts[5], GenerateUniqueID().c_str());

	entity = new XMLEntity("presence", (const char **)atts);

	// send XML command
	char *str = entity->ToString();
	socketAdapter->SendData(BString(str));
	free(str);
	
	DestroyAttributeMemory(atts, 6);
	delete entity;
}

void
JabberProtocol::AddToRoster(UserID *new_user)
{
	/*
	<iq from='juliet@example.com/balcony' type='set' id='roster_2'>
     <query xmlns='jabber:iq:roster'>
       <item jid='nurse@example.com'
             name='Nurse'>
         <group>Servants</group>
       </item>
     </query>
   </iq>
   */
   
	BString xml = "<iq type='set'>"
		"<query xmlns='jabber:iq:roster'>"
		"<item jid='";
	xml.Append(new_user->Handle().c_str());
	xml << "' name='";
	xml.Append(new_user->FriendlyName().c_str());
	xml << "' subscription='to'>";
	
	if (new_user->UserType() == UserID::CONFERENCE)
	{
		xml << "<group>#Conference</group>";
	}
	
	xml << "</item></query></iq>";
	socketAdapter->SendData(xml);
   
}

void
JabberProtocol::RemoveFromRoster(UserID *removed_user)
{
	XMLEntity *entity, *entity_query, *entity_item;

	char **atts       = CreateAttributeMemory(4);
	char **atts_query = CreateAttributeMemory(2);
	char **atts_item  = CreateAttributeMemory(6);
	
	// assemble attributes
	strcpy(atts[0], "type");
	strcpy(atts[1], "set");
	strcpy(atts[2], "id");
	strcpy(atts[3], GenerateUniqueID().c_str());

	strcpy(atts_query[0], "xmlns");
	strcpy(atts_query[1], "jabber:iq:roster");

	strcpy(atts_item[0], "jid");
	strcpy(atts_item[1], removed_user->Handle().c_str());
	strcpy(atts_item[2], "name");
	strcpy(atts_item[3], removed_user->FriendlyName().c_str());
	strcpy(atts_item[4], "subscription");
	strcpy(atts_item[5], "remove");

	entity = new XMLEntity("iq", (const char **)atts);
	entity_query = new XMLEntity("query", (const char **)atts_query);
	entity_item = new XMLEntity("item", (const char **)atts_item);

	entity_query->AddChild(entity_item);
	entity->AddChild(entity_query);

	// log command
	//_iq_map[atts[3]] = ROSTER;

	// send XML command
	char *str = entity->ToString();
	socketAdapter->SendData(BString(str));
	free(str);
	
	DestroyAttributeMemory(atts, 4);
	DestroyAttributeMemory(atts_query, 2);
	DestroyAttributeMemory(atts_item, 6);
	
	delete entity;
}

void
JabberProtocol::ProcessUserPresence(UserID *user, XMLEntity *entity)
{
	char buffer[4096];
	
	// get best asker name
	const char *asker;
					
	if (user && user->FriendlyName().size() > 0) {
		// they have a friendly name
		asker = user->FriendlyName().c_str();
	} else if (entity->Attribute("from")) {
		// they have a JID
		asker = entity->Attribute("from");
	} else {
		// they have no identity (illegal case)
		asker = "<unknown>";
	}
	
	// get presence
	const char *availability = NULL;

	if (entity->Attribute("type")) {
		availability = entity->Attribute("type");
	} else {
		availability = "available";
	}
	
	//fprintf(stderr, "Presence type: %s.\n", availability);

	// reflect presence
	if (user && !strcasecmp(availability, "unavailable"))
	{
		user->SetOnlineStatus(UserID::OFFLINE);
		fprintf(stderr, "User %s is unavailable.\n", user->JabberHandle().c_str());
	}
	else if (user && !strcasecmp(availability, "available"))
	{
		user->SetOnlineStatus(UserID::ONLINE);
		fprintf(stderr, "User %s is available.\n", user->JabberHandle().c_str());
	}
	else if (!strcasecmp(availability, "unsubscribe"))
	{
		fprintf(stderr, "User %s is unsubscribed from you.\n", user->JabberHandle().c_str());
		sprintf(buffer, "User %s no longer wishes to know your online status.", asker);
		ModalAlertFactory::NonModalAlert(buffer, "I feel so unloved.");
	}
	else if (user && !strcasecmp(availability, "unsubscribed"))
	{
		// do nothing?
		user->SetOnlineStatus(UserID::UNKNOWN);
	}
	else if (user && !strcasecmp(availability, "subscribed"))
	{
		user->SetOnlineStatus(UserID::ONLINE);

		if (entity->Child("status")) {
			sprintf(buffer, "[%s]\n\n%s", asker, entity->Child("status")->Data());
		} else {
			sprintf(buffer, "Your subscription request was accepted by %s!", asker);
		}
		
		ModalAlertFactory::Alert(buffer, "Hooray!");
	}
	else if (!strcasecmp(availability, "subscribe"))
	{
		sprintf(buffer, "%s would like to subscribe to your presence so they may know if you're online or not.  Would you like to allow it?", asker);
		
		fprintf(stderr, "User %s want to subscribe to you.\n", asker);

		// query for presence authorization (for users)
		int32 answer = 0;
				
		if (user->IsUser()) {
			answer = ModalAlertFactory::Alert(buffer, "No, I prefer privacy.", "Yes, grant them my presence!");
		} else if (user->UserType() == UserID::TRANSPORT) {
			answer = 1;
		}

		// send back the response
		if (answer == 1) {
			// presence is granted
			AcceptPresence(entity->Attribute("from"));
		} else if (answer == 0) {
			// presence is denied
			RejectPresence(entity->Attribute("from"));
		}
	}
	else if (!strcasecmp(availability, "error"))
	{
		if (entity->Child("error") && entity->Child("error")->Child("text"))
		{
			string username = entity->Attribute("from");
			fprintf(stderr, "Presence Error: %s.\n", entity->Child("error")->Child("text")->Data());
			sprintf(buffer, "Following error occured from %s:\n\n %s.", username.c_str(), entity->Child("error")->Child("text")->Data());
		
			ModalAlertFactory::NonModalAlert(buffer, "Sad :(");
		}
	}

	if (user && (!strcasecmp(availability, "available") ||
		!strcasecmp(availability, "unavailable")))
	{
		if (entity->Child("show") && entity->Child("show")->Data()) {
			user->SetExactOnlineStatus(entity->Child("show")->Data());
		} else {
			user->SetExactOnlineStatus("chat");
			fprintf(stderr, "Process User Presence show %s.\n", "chat");
		}	

		if (entity->Child("status") && entity->Child("status")->Data()) {
			user->SetMoreExactOnlineStatus(entity->Child("status")->Data());
		} else
			user->SetMoreExactOnlineStatus("");
		
	}
}

void
JabberProtocol::ParseRosterList(XMLEntity *iq_roster_entity)
{
	XMLEntity *entity = iq_roster_entity;
	
	// go one level deep to query
	if (entity->Child("query")) {
		entity = entity->Child("query");
	} else {
		return;
	}

	// iterate through child 'item' tags
	JRoster::Instance()->Lock();
	
	for (int i=0; i<entity->CountChildren(); ++i) {
		
		// handle the item child
		if (!strcasecmp(entity->Child(i)->Name(), "item")) {
			if (!entity->Child(i)->Attribute("jid")) {
				continue;
			}

			// make a user
			UserID user(entity->Child(i)->Attribute("jid"));
			
			fprintf(stderr,  "Roster item %s.\n", user.JabberHandle().c_str());
		
			if (entity->Child(i)->Child("group")) {
				if (!strcasecmp(entity->Child(i)->Child("group")->Data(), "#Conference"))
				{
					user.SetUsertype(UserID::CONFERENCE);
					user.SetOnlineStatus(UserID::CONF_STATUS);
					fprintf(stderr, "CONFERENCE founded.\n");
				}
			}

			// set friendly name
			if (entity->Child(i)->Attribute("name")) {
				user.SetFriendlyName(entity->Child(i)->Attribute("name"));
			}
			
			// set subscription status
			if (entity->Child(i)->Attribute("subscription")) {
				user.SetSubscriptionStatus(entity->Child(i)->Attribute("subscription"));
			}
/*
			// set ask
			if (entity->Child(i)->Attribute("ask")) {
				user.SetAsk(entity->Child(i)->Attribute("ask"));
			}
*/
			// obtain a handle to the user (is there a new one?)
			UserID *roster_user;
			
			if (user.IsUser()) {
				roster_user = JRoster::Instance()->FindUser(JRoster::HANDLE, user.JabberHandle());
			} /*else if (user.UserType() == UserID::TRANSPORT) {
				roster_user = JRoster::Instance()->FindUser(JRoster::TRANSPORT_ID, user.TransportID());
			} else {
				continue;
			}*/

			// if we have duplicates, settle disputes
			if (roster_user) {
				// process if it's a removal
				if (entity->Child(i)->Attribute("subscription") && 
						!strcasecmp(entity->Child(i)->Attribute("subscription"), "remove")) {
					// remove from the list
					JRoster::Instance()->RemoveUser(roster_user);

					continue;
				}

				// update the new roster item
				*roster_user = user;
			} else {
				// create the user
				roster_user = new UserID(entity->Child(i)->Attribute("jid"));

				*roster_user = user;
				
				// add to the list
				JRoster::Instance()->AddRosterUser(roster_user);
				
			}
		}
	}

	JRoster::Instance()->Unlock();	
	
	mainWindow->PostMessage(BLAB_UPDATE_ROSTER);

	
}

static int32 SessionDispatcher(void *args) 
{
	JabberProtocol *jabber = (JabberProtocol*)args;
	BMessage msg(PORT_TALKER_DATA);
	
	while (true)
	{
		acquire_sem(jabber->read_queue);
				
		jabber->ReceiveData(&msg);
		jabber->ReceivedMessageHandler(&msg);
		msg.MakeEmpty();
		
		release_sem(jabber->read_queue);
		
		snooze(5000);
	}
	
	return 0;
}

void
JabberProtocol::ReceiveData(BMessage *msg)
{
	BMessage packet(PORT_TALKER_DATA);
	BString msgData;
	
	bool found_stream_start = false;
	bool found_stream_end = false;
	
	int no = 0;
	
	do 
	{
		BString data;
		int32 length;
		
		packet.MakeEmpty();
		socketAdapter->ReceiveData(&packet);
	
		packet.FindString("data", &data);
		packet.FindInt32("length", &length);

		if (data.FindFirst("<stream:stream") >= 0)
			found_stream_start = true;
			
		if (data.FindFirst("</stream:stream") >= 0)
			found_stream_end = true;
			
		msgData.Append(data);
		
#ifdef DEBUG

		//fprintf(stderr, "IQ PACKET %i LEN: %i.\n", no++, length);
			
#endif

	} while (CheckXML(msgData.String()) == NULL && 
				!found_stream_start && !found_stream_end);
	
	msg->AddInt32("length", msgData.Length());
	
	msgData = BString(FXMLSkipHead(msgData.String())).Prepend("<dengon>").Append("</dengon>");
	
	msg->AddString("data", msgData);
	
}

void
JabberProtocol::ReceivedMessageHandler(BMessage *msg)
{
	BString data;
	msg->FindString("data", &data);
	int length = msg->FindInt32("length");
	
#ifdef DEBUG

		fprintf(stderr, "\nDATA received %i: %s\n", (int)data.Length(), data.String());

#endif

	Reset();
	LockXMLReader();
	FeedData(data.String(), data.Length());
	UnlockXMLReader();

}



void
JabberProtocol::SendMessage(BString to, BString text)
{
	/*
	BString xml = "<message type='chat' to='";
	xml = xml.Append(to);
	xml << "'><body>";
	xml = xml.Append(text);
	xml << "</body></message>";
	
	socketAdapter->SendData(xml);
	*/
	
	XMLEntity   *entity;
	char **atts = CreateAttributeMemory(4);

	// assemble attributes;
	strcpy(atts[0], "to");
	strcpy(atts[1], to.String());
	strcpy(atts[2], "type");
	strcpy(atts[3], "chat");
	
	// construct XML tagset
	entity = new XMLEntity("message", (const char **)atts);

	entity->AddChild("body", NULL, text.String());
	//entity->AddChild("thread", NULL, thread_id.c_str());

	// send XML command
	char *str = entity->ToString();
	socketAdapter->SendData(str);
	free(str);

	DestroyAttributeMemory(atts, 4);
	
	delete entity;
}

void
JabberProtocol::SendGroupchatMessage(BString to, BString text)
{
	/*
	BString xml = "<message type='groupchat' to='";
	xml = xml.Append(to);
	xml << "'><body>";
	xml = xml.Append(text);
	xml << "</body></message>";
	
	socketAdapter->SendData(xml);
	*/
	XMLEntity   *entity;
	char **atts = CreateAttributeMemory(4);

	// assemble attributes;
	strcpy(atts[0], "to");
	strcpy(atts[1], to.String());
	strcpy(atts[2], "type");
	strcpy(atts[3], "groupchat");
	
	// construct XML tagset
	entity = new XMLEntity("message", (const char **)atts);

	entity->AddChild("body", NULL, text.String());

	// send XML command
	char *str = entity->ToString();
	socketAdapter->SendData(str);
	free(str);

	DestroyAttributeMemory(atts, 4);
	
	delete entity;
}

void 
JabberProtocol::RequestInfo()
{
	BString xml;
	xml << "<iq type='get' to='gmail.com'><query xmlns='http://jabber.org/protocol/disco#info'/></iq>";
	
	socketAdapter->SendData(xml);
}

void
JabberProtocol::RequestRoster() 
{
	acquire_sem(logged);
	release_sem(logged);
		
	BString xml;
	
	xml << "<iq id='roster_2' from='";
	xml = xml.Append(jid);
	xml << "' type='get'><query xmlns='jabber:iq:roster'/></iq>";
		
	socketAdapter->SendData(xml);
	
}

void
JabberProtocol::SendMUCConferenceRequest(BString conference)
{
	BString xml = "<iq from='";
	xml = xml.Append(jid);
	xml << "' id='disco1' to='";
	xml = xml.Append(conference);
	xml << "' type='get'><query xmlns='http://jabber.org/protocol/disco#info'/></iq>";
	
	socketAdapter->SendData(xml);
}

void
JabberProtocol::SendMUCRoomRequest(BString room)
{
	/*
		<iq from='hag66@shakespeare.lit/pda'
    	id='disco3'
    	to='darkcave@chat.shakespeare.lit'
    	type='get'>
  		<query xmlns='http://jabber.org/protocol/disco#info'/>
		</iq>
	*/
	
	BString xml = "<iq from='";
	xml = xml.Append(jid);
	xml << "' id='disco3' to='";
	xml = xml.Append(room);
	xml << "' type='get'><query xmlns='http://jabber.org/protocol/disco#info'/></iq>";
	
	socketAdapter->SendData(xml);
}

void
JabberProtocol::InitSession() 
{
	BString xml = "<stream:stream to='";
	xml = xml.Append(domain);
	xml << "' "
		"xmlns='jabber:client' "
		"xmlns:stream='http://etherx.jabber.org/streams' "
		"version='1.0'>";
	
	socketAdapter->SendData(xml);
}

void
JabberProtocol::Session()
{
	BString xml = "<iq to='";
	xml = xml.Append(domain);
	xml << "' type='set' id='sess_1'>"
				"<session xmlns='urn:ietf:params:xml:ns:xmpp-session'/></iq>";
	
	socketAdapter->SendData(xml);
}

void
JabberProtocol::Bind() 
{
	BString xml = "<iq type='set' id='bind_0'>"
  		"<bind xmlns='urn:ietf:params:xml:ns:xmpp-bind'>"
    	"<resource>haiku</resource>"
    	"</bind>"
		"</iq>";

	socketAdapter->SendData(xml);
}

bool
JabberProtocol::BeginSession() 
{
	if (host == "" || domain == "" || user == "" || pass == "" || port <= 0)
		return false;

	socketAdapter->Create();
	socketAdapter->Open(host, port);
	
	if (secure)
		socketAdapter->StartTLS();
		
	if (socketAdapter->IsConnected())
	{
		InitSession();
		return true;
	}

	char buffer[50 + host.Length()];
	sprintf(buffer, "Cannot connect to %s:%i.", host.String(), port);
	ModalAlertFactory::Alert(buffer, "Sorry", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT); 
	
	Disconnect();

	return false;
}

// SASL PLAIN

bool
JabberProtocol::Authorize()
{
	int length = (strlen(user)*2)+strlen(domain)+strlen(pass)+3;
	char credentials[length];
	sprintf(credentials, "%s@%s%c%s%c%s", user.String(), domain.String(), '\0', user.String(), '\0', pass.String());
	
	BString xml = "<auth xmlns='urn:ietf:params:xml:ns:xmpp-sasl' mechanism='PLAIN'>";
	
	char creds64[length*4];
	
	Base64Encode(credentials, length, creds64, length*4);
	xml = xml.Append(creds64);
	xml << "</auth>";
					
	socketAdapter->SendData(xml);
	
	return false;
}
