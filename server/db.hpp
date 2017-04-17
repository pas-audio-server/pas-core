#pragma once
#include <string>

/*  This file is part of pas.

    pas is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    pas is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with pas.  If not, see <http://www.gnu.org/licenses/>.
*/

/*	Copyright 2017 by Perry Kivolowitz
*/

// WARNING
// WARNING
// WARNING THIS ARRAY IS SHARED BETWEEN THE PAS SERVER AND THE MEDIA
// WARNING DISCOVERY SYSTEM - FSMAIN.  IT  IS ALSO DEPENDENT UPON AN
// WARNING EXTERN DATABASE  SCHEMA  (ALL COLUMN NAMES ARE PRESENT IN
// WARNING IN THE DATABASE).
// WARNING
// WARNING

std::string track_column_names[] =
{
	// Order must match select / insert code
	"parent",
	"fname",
	"namespace",
	"artist",
	"title",
	"album",
	"genre",
	"source",
	"duration",
	"publisher",
	"composer",
	"track",
	"copyright",
	"disc"
};
