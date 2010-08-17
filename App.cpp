#include "App.h"
#include "MainWindow.h"
#include "TalkManager.h"
#include "MessageRepeater.h"
#include "ChatWindow.h"
#include "SecureSocket.h"
#include "JabberProtocol.h"



App::App(void)
	:	BApplication("application/x-vnd.Be-elfexecutable")
{
	
	MessageRepeater::Instance()->Run();
	JabberProtocol *jabber = new JabberProtocol();
	jabber->mainWindow = BlabberMainWindow::Instance();
	TalkManager::Instance()->jabber = jabber;
	BlabberMainWindow::Instance()->jabber = jabber;
	BlabberMainWindow::Instance()->Show();
}

void
App::AboutRequested()
{
	BAlert *alert = new BAlert("about", "Chat\n\n"
		"By Maxim Sokhatsky.\n"
		"Based on Jabber for BeOS by John Blanco.\n\n", "Chat!");
	BTextView *view = alert->TextView();
	BFont font;
	view->SetStylable(true);
	view->GetFont(&font);
	font.SetSize(font.Size()+7.0f);
	font.SetFace(B_BOLD_FACE);
	view->SetFontAndColor(0,4,&font);
	alert->Go();
}

int
main(void)
{
	App *app = new App();
	app->Run();
	delete app;
	return 0;
}
