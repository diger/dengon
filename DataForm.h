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
#include <interface/Window.h>
#include <interface/CheckBox.h>
#include <interface/TextControl.h>
#include <interface/ScrollView.h>
#include <String.h>
#include "XMLEntity.h"

class DataFormView : BWindow
{
	public:
							DataFormView(BRect rect);
							~DataFormView();

				XMLEntity	*entity;
				BView *xform;
				int leftSize;
		void		LoadDataForm(XMLEntity *entity);
		void ShowWindow();
		virtual void		FrameResized(float width, float height);

};

#endif
