#include "track.hpp"

using namespace std;

void Track::SetPath(string & path)
{
	string s("path");
	SetTag(s, path);
}

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
	transform(tag.begin(), tag.end(), tag.begin(), ::tolower);
	tags[tag] = value;
}

string Track::GetTag(string & tag)
{
	map<string, string>::iterator it;

	it = tags.find(tag);
	return (it != tags.end()) ? it->second : "";
}

void Track::PrintTags(int width_1, int width_2)
{
	for (auto it = tags.begin(); it != tags.end(); it++)
	{
		cout << left << setw(width_1) << it->first << setw(width_2) << it->second << endl;
	}
}

