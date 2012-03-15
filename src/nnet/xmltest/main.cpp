#include "../xml.h"
#include "../stream/bang_fstream.h"
#include "../stream/bang_iostream.h"

void main ()
{
	XMLFile x;
	x.setfile ("log.txt");
	CXml *xml = x.parse();
	xml->AddIncludes(".");
	xml->AddDefaults();

	bang_ofstream f ("log.txt");
	f << CString (*xml);
	f.close();
	delete xml;
	//while (true);
}
