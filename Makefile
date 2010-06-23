NAME= Gossip
TYPE= APP
SRCS= App.cpp JRoster.cpp TransportItem.cpp RosterItem.cpp UserID.cpp BuddyWindow.cpp TalkManager.cpp StatusView.cpp Agent.cpp RosterView.cpp AppLocation.cpp RosterSuperitem.cpp AgentList.cpp ModalAlertFactory.cpp XMLEntity.cpp Settings.cpp MessageRepeater.cpp XMLReader.cpp FileXMLReader.cpp MainWindow.cpp ChatWindow.cpp EditingFilter.cpp GenericFunctions.cpp Socket.cpp SecureSocket.cpp JabberProtocol.cpp SplitView.cpp Base64.cpp
RSRCS= Resources.rsrc
LIBS= /boot/develop/lib/x86/libbe.so /boot/develop/lib/x86/libroot.so /boot/common/lib/libssl.so /boot/common/lib/libcrypto.so /boot/develop/lib/x86/libnetwork.so /boot/develop/lib/x86/libtranslation.so /boot/common/lib/libexpat.so /boot/develop/lib/x86/libstdc++.r4.so
LIBPATHS=
SYSTEM_INCLUDE_PATHS= /boot/develop/headers/be
LOCAL_INCLUDE_PATHS=
OPTIMIZE=NONE
#	specify any preprocessor symbols to be defined.  The symbols will not
#	have their values set automatically; you must supply the value (if any)
#	to use.  For example, setting DEFINES to "DEBUG=1" will cause the
#	compiler option "-DDEBUG=1" to be used.  Setting DEFINES to "DEBUG"
#	would pass "-DDEBUG" on the compiler's command line.
DEFINES=
#	specify special warning levels
#	if unspecified default warnings will be used
#	NONE = supress all warnings
#	ALL = enable all warnings
WARNINGS =
# Build with debugging symbols if set to TRUE
SYMBOLS=
COMPILER_FLAGS=
LINKER_FLAGS=

## include the makefile-engine
include $(BUILDHOME)/etc/makefile-engine
