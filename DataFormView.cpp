#include "DataFormView.h"


DataFormView::DataFormView()
: BView("DataFormView", B_FOLLOW_LEFT_RIGHT)
{
	
}

DataFormView::~DataFormView()
{
}

void
DataFormView::LoadDataForm(XMLEntity *_entity)
{
	entity = _entity;
}

