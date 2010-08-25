//////////////////////////////////////////////////
//
// Haiku Chat [DataFormView.h]
//
//     XEP-0004 Data Forms implementation.
//
// Copyright (c) 2010 Maxim Sokhatsky (maxim.sokhatsky@gmail.com)
//
//////////////////////////////////////////////////

#ifndef DATAFORMVIEW_H
#define DATAFORMVIEW_H

#include <interface/View.h>
#include "XMLEntity.h"

class DataFormView : BView
{
	public:
							DataFormView();
							~DataFormView();

				XMLEntity	*entity;
		void		LoadDataForm(XMLEntity *entity);
};

#endif
