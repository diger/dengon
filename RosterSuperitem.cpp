//////////////////////////////////////////////////
// Blabber [RosterSuperitem.cpp]
//////////////////////////////////////////////////

#ifndef ROSTER_SUPERITEM_H
	#include "RosterSuperitem.h"
#endif
#include <iostream>
RosterSuperitem::RosterSuperitem(const char *text)
	: BStringItem(text) {
}

RosterSuperitem::~RosterSuperitem() {
}

void RosterSuperitem::DrawItem(BView *owner, BRect frame, bool complete) {
	owner->SetFontSize(10.0);
	owner->SetHighColor(0, 0, 0, 255);

	BStringItem::DrawItem(owner, frame, complete);
}