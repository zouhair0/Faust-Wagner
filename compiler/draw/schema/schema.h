/************************************************************************
 ************************************************************************
    FAUST compiler
	Copyright (C) 2003-2004 GRAME, Centre National de Creation Musicale
    ---------------------------------------------------------------------
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 ************************************************************************
 ************************************************************************/

#ifndef __SCHEMA__
#define __SCHEMA__


#include "device.h"
#include <vector>
#include <string>

using namespace std;

const double dWire = 8; 			///< distance between two wires
const double dLetter = 6;			///< width of a letter
const double dHorz = 4;       		///< marge horizontale
const double dVert = 4;				///< marge verticale


struct point
{
	double x;
	double y;

	point() : x(0.0), y(0.0) {}
	point(double f) : x(f),y(f) {}
	point(double u, double v) : x(u), y(v) {}
	point(const point& p) : x(p.x), y(p.y) {}
};

enum { kLeftRight=1, kRightLeft=-1 };


/**
 * An abstract block diagram schema
 */
class schema
{
  private:
	const unsigned int	fInputs;
	const unsigned int	fOutputs;
	const double		fWidth;
	const double		fHeight;

	// fields only defined after place() is called
  	bool			fPlaced;		///< false until place() is called
	double 			fX;
	double 			fY;
	int				fOrientation;

  public:

 	schema(unsigned int inputs, unsigned int outputs, double width, double height)
 	 	: 	fInputs(inputs),
 	 		fOutputs(outputs),
 	 		fWidth(width),
 	 		fHeight(height),
 	 		fPlaced(false)
 	{}
  	virtual ~schema() {}

 	// constant fields
 	double			width()				{ return fWidth; }
 	double			height()			{ return fHeight; }
 	unsigned int	inputs()			{ return fInputs; }
 	unsigned int	outputs()			{ return fOutputs; }

 	// starts and end placement
	void 			beginPlace (double x, double y, int orientation)
							{ fX = x; fY = y; fOrientation = orientation; }
 	void			endPlace ()			{ fPlaced = true; }

 	// fields available after placement
 	bool			placed()			{ return fPlaced; }
 	double 			x()					{ return fX; }
	double 			y()					{ return fY; }
	int				orientation()		{ return fOrientation; }


 	// abstract interface for subclasses
	virtual void 	place(double x, double y, int orientation) 	= 0;
	virtual void 	draw(device& dev) 							= 0;
	virtual point	inputPoint(unsigned int i) 					= 0;
	virtual point 	outputPoint(unsigned int i) 				= 0;
};

// various functions to create schemas

schema* makeBlockSchema 	(unsigned int inputs,
							 unsigned int outputs,
  							 const string& name,
							 const string& color,
							 const string& link);

schema* makeCableSchema 	(unsigned int n=1);
schema* makeCutSchema 		();
schema* makeEnlargedSchema 	(schema* s, double width);
schema* makeParSchema 		(schema* s1, schema* s2);
schema* makeSeqSchema 		(schema* s1, schema* s2);
schema* makeMergeSchema 	(schema* s1, schema* s2);
schema* makeSplitSchema 	(schema* s1, schema* s2);
schema* makeRecSchema 		(schema* s1, schema* s2);
schema* makeTopSchema 		(schema* s1, double margin, const string& text, const string& link);
schema* makeDecorateSchema 	(schema* s1, double margin, const string& text);









#endif

