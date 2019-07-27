




















#ifndef rdfutil_h__
#define rdfutil_h__


class nsACString;
class nsCString;

nsresult
rdf_MakeRelativeRef(const nsCSubstring& aBaseURI, nsCString& aURI);

void
rdf_FormatDate(PRTime aTime, nsACString &aResult);

PRTime
rdf_ParseDate(const nsACString &aTime);

#endif 


