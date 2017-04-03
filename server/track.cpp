#include "track.hpp"

using namespace std;

void Track::SetPath(string & path)
{
	string s("path");
	SetTag(s, path);
}

/*	SetTag() parses a metadata tag produced by ffprobe. Metadata
	looks like:

	TAG:tagname=tagvalue\n.

	Did you know regex was invented by the second Chair of the
	UW - Madisonn CS department?

	After parsing the tag, the key-value pair is added to the
	track's tag map.
*/
void Track::SetTag(string & raw)
{
	regex c("TAG:.*=.*\n");
	regex t(":.*=");
	regex v("=.*\n");

	if (regex_search(raw, c))
	{
		smatch mtag;
		smatch mval;
		string tag, value;

		regex_search(raw, mtag, t);
		regex_search(raw, mval, v);
		if (mtag.str(0).size() > 2)
			tag = string(mtag.str(0), 1, mtag.str(0).size() - 2);
		if (mval.str(0).size() > 2)
			value = string(mval.str(0), 1, mval.str(0).size() - 2);
		SetTag(tag, value);
	}
}

void Track::SetTag(string & tag, string & value)
{
	// All keys are squashed to lower case.
	transform(tag.begin(), tag.end(), tag.begin(), ::tolower);
	tags[tag] = value;
}

string Track::GetTag(string & tag)
{
	map<string, string>::iterator it;

	it = tags.find(tag);
	return (it != tags.end()) ? it->second : "";
}

/*	PrintTags() may be of some use during debugging.
*/
void Track::PrintTags(int width_1, int width_2)
{
	for (auto it = tags.begin(); it != tags.end(); it++)
	{
		cout << left << setw(width_1) << it->first << setw(width_2) << it->second << endl;
	}
}

