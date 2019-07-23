










































#include "nsLocalSearchService.h"
#include "nscore.h"
#include "nsIServiceManager.h"
#include "nsIRDFContainerUtils.h"
#include "nsEnumeratorUtils.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "plhash.h"
#include "plstr.h"
#include "prmem.h"
#include "prprf.h"
#include "prio.h"
#include "prlog.h"
#include "nsITextToSubURI.h"
#include "nsIRDFObserver.h"
#include "nsRDFCID.h"
#include "rdf.h"
#include "nsCRT.h"

static NS_DEFINE_CID(kRDFServiceCID,               NS_RDFSERVICE_CID);
static NS_DEFINE_CID(kTextToSubURICID,             NS_TEXTTOSUBURI_CID);

static	nsIRDFService		*gRDFService = nsnull;
static	LocalSearchDataSource		*gLocalSearchDataSource = nsnull;
static const char	kFindProtocol[] = "find:";

static PRBool
isFindURI(nsIRDFResource *r)
{
	PRBool		isFindURIFlag = PR_FALSE;
	const char	*uri = nsnull;
	
	r->GetValueConst(&uri);
	if ((uri) && (!strncmp(uri, kFindProtocol, sizeof(kFindProtocol) - 1)))
	{
		isFindURIFlag = PR_TRUE;
	}
	return(isFindURIFlag);
}


PRInt32              LocalSearchDataSource::gRefCnt;
nsIRDFResource		*LocalSearchDataSource::kNC_Child;
nsIRDFResource		*LocalSearchDataSource::kNC_Name;
nsIRDFResource		*LocalSearchDataSource::kNC_URL;
nsIRDFResource		*LocalSearchDataSource::kNC_FindObject;
nsIRDFResource		*LocalSearchDataSource::kNC_pulse;
nsIRDFResource		*LocalSearchDataSource::kRDF_InstanceOf;
nsIRDFResource		*LocalSearchDataSource::kRDF_type;

LocalSearchDataSource::LocalSearchDataSource(void)
{
	if (gRefCnt++ == 0)
	{
		nsresult rv = CallGetService(kRDFServiceCID, &gRDFService);

		PR_ASSERT(NS_SUCCEEDED(rv));

		gRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "child"),
                             &kNC_Child);
		gRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "Name"),
                             &kNC_Name);
		gRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "URL"),
                             &kNC_URL);
		gRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "FindObject"),
                             &kNC_FindObject);
		gRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "pulse"),
                             &kNC_pulse);

		gRDFService->GetResource(NS_LITERAL_CSTRING(RDF_NAMESPACE_URI "instanceOf"),
                             &kRDF_InstanceOf);
		gRDFService->GetResource(NS_LITERAL_CSTRING(RDF_NAMESPACE_URI "type"),
                             &kRDF_type);

		gLocalSearchDataSource = this;
	}
}



LocalSearchDataSource::~LocalSearchDataSource (void)
{
	if (--gRefCnt == 0)
	{
		NS_RELEASE(kNC_Child);
		NS_RELEASE(kNC_Name);
		NS_RELEASE(kNC_URL);
		NS_RELEASE(kNC_FindObject);
		NS_RELEASE(kNC_pulse);
		NS_RELEASE(kRDF_InstanceOf);
		NS_RELEASE(kRDF_type);

		gLocalSearchDataSource = nsnull;
		NS_RELEASE(gRDFService);
	}
}



nsresult
LocalSearchDataSource::Init()
{
	nsresult	rv = NS_ERROR_OUT_OF_MEMORY;

	
	if (NS_FAILED(rv = gRDFService->RegisterDataSource(this, PR_FALSE)))
		return(rv);

	return(rv);
}



NS_IMPL_ISUPPORTS1(LocalSearchDataSource, nsIRDFDataSource)



NS_IMETHODIMP
LocalSearchDataSource::GetURI(char **uri)
{
	NS_PRECONDITION(uri != nsnull, "null ptr");
	if (! uri)
		return NS_ERROR_NULL_POINTER;

	if ((*uri = nsCRT::strdup("rdf:localsearch")) == nsnull)
		return NS_ERROR_OUT_OF_MEMORY;

	return NS_OK;
}



NS_IMETHODIMP
LocalSearchDataSource::GetSource(nsIRDFResource* property,
                          nsIRDFNode* target,
                          PRBool tv,
                          nsIRDFResource** source )
{
	NS_PRECONDITION(property != nsnull, "null ptr");
	if (! property)
		return NS_ERROR_NULL_POINTER;

	NS_PRECONDITION(target != nsnull, "null ptr");
	if (! target)
		return NS_ERROR_NULL_POINTER;

	NS_PRECONDITION(source != nsnull, "null ptr");
	if (! source)
		return NS_ERROR_NULL_POINTER;

	*source = nsnull;
	return NS_RDF_NO_VALUE;
}



NS_IMETHODIMP
LocalSearchDataSource::GetSources(nsIRDFResource *property,
                           nsIRDFNode *target,
			   PRBool tv,
                           nsISimpleEnumerator **sources )
{
	NS_NOTYETIMPLEMENTED("write me");
	return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
LocalSearchDataSource::GetTarget(nsIRDFResource *source,
                          nsIRDFResource *property,
                          PRBool tv,
                          nsIRDFNode **target )
{
	NS_PRECONDITION(source != nsnull, "null ptr");
	if (! source)
		return NS_ERROR_NULL_POINTER;

	NS_PRECONDITION(property != nsnull, "null ptr");
	if (! property)
		return NS_ERROR_NULL_POINTER;

	NS_PRECONDITION(target != nsnull, "null ptr");
	if (! target)
		return NS_ERROR_NULL_POINTER;

	nsresult		rv = NS_RDF_NO_VALUE;

	
	if (! tv)
		return rv;

	if (isFindURI(source))
	{
		if (property == kNC_Name)
		{

		}
		else if (property == kNC_URL)
		{
			

			nsAutoString	url;
			nsIRDFLiteral	*literal;
			gRDFService->GetLiteral(url.get(), &literal);
			*target = literal;
			return(NS_OK);
		}
		else if (property == kRDF_type)
		{
			const char	*uri = nsnull;
			rv = kNC_FindObject->GetValueConst(&uri);
			if (NS_FAILED(rv)) return rv;

			nsAutoString	url; url.AssignWithConversion(uri);
			nsIRDFLiteral	*literal;
			gRDFService->GetLiteral(url.get(), &literal);

			*target = literal;
			return(NS_OK);
		}
		else if (property == kNC_pulse)
		{
			nsAutoString	pulse(NS_LITERAL_STRING("15"));
			nsIRDFLiteral	*pulseLiteral;
			rv = gRDFService->GetLiteral(pulse.get(), &pulseLiteral);
			if (NS_FAILED(rv)) return rv;

			*target = pulseLiteral;
			return(NS_OK);
		}
		else if (property == kNC_Child)
		{
			
			
			*target = source;
			NS_ADDREF(*target);
			return(NS_OK);
		}
	}
	return NS_RDF_NO_VALUE;
}



NS_METHOD
LocalSearchDataSource::parseResourceIntoFindTokens(nsIRDFResource *u, findTokenPtr tokens)
{
	const char		*uri = nsnull;
	char			*id, *token, *value, *newstr;
	int			loop;
	nsresult		rv;

	if (NS_FAILED(rv = u->GetValueConst(&uri)))	return(rv);

#ifdef	DEBUG
	printf("Find: %s\n", (const char*) uri);
#endif

	if (!(id = PL_strdup(uri + sizeof(kFindProtocol) - 1)))
		return(NS_ERROR_OUT_OF_MEMORY);

	
	if ((token = nsCRT::strtok(id, "&", &newstr)) != NULL)
	{
		while (token != NULL)
		{
			if ((value = strstr(token, "=")) != NULL)
			{
				*value++ = '\0';
			}
			for (loop=0; tokens[loop].token != NULL; loop++)
			{
				if (!strcmp(token, tokens[loop].token))
				{
				    if (!strcmp(token, "text"))
				    {
            			nsCOMPtr<nsITextToSubURI> textToSubURI = 
            			         do_GetService(kTextToSubURICID, &rv);
            			if (NS_SUCCEEDED(rv) && (textToSubURI))
            			{
            				PRUnichar	*uni = nsnull;
            				if (NS_SUCCEEDED(rv = textToSubURI->UnEscapeAndConvert("UTF-8", value, &uni)) && (uni))
            				{
    					        tokens[loop].value = uni;
    					        NS_Free(uni);
    					    }
    					}
				    }
				    else
				    {
				        nsAutoString    valueStr;
				        valueStr.AssignWithConversion(value);
				        tokens[loop].value = valueStr;
    			    }
					break;
				}
			}
			token = nsCRT::strtok(newstr, "&", &newstr);
		}
	}
	PL_strfree(id);
	return(NS_OK);
}



PRBool
LocalSearchDataSource::doMatch(nsIRDFLiteral *literal,
                               const nsAString &matchMethod,
                               const nsString &matchText)
{
	PRBool		found = PR_FALSE;

	if ((nsnull == literal) ||
            matchMethod.IsEmpty() ||
            matchText.IsEmpty())
		return(found);

	const	PRUnichar	*str = nsnull;
	literal->GetValueConst( &str );
	if (! str)	return(found);
	nsAutoString	value(str);

        if (matchMethod.EqualsLiteral("contains"))
	{
            if (FindInReadable(matchText, value,
                               nsCaseInsensitiveStringComparator()))
                found = PR_TRUE;
	}
        else if (matchMethod.EqualsLiteral("startswith"))
	{
            nsAString::const_iterator start, realstart, end;
            value.BeginReading(start);
            value.EndReading(end);
            realstart = start;
            
            if (FindInReadable(matchText, start, end,
                               nsCaseInsensitiveStringComparator()) &&
                start == realstart)
                
                found = PR_TRUE;
	}
        else if (matchMethod.EqualsLiteral("endswith"))
	{
            nsAString::const_iterator start, end, realend;
            value.BeginReading(start);
            value.EndReading(end);
            realend = end;

            if (RFindInReadable(matchText, start, end,
                                nsCaseInsensitiveStringComparator()) &&
                end == realend)
                
                found = PR_TRUE;
	}
        else if (matchMethod.EqualsLiteral("is"))
	{
            if (value.Equals(matchText, nsCaseInsensitiveStringComparator()))
                found = PR_TRUE;
	}
        else if (matchMethod.EqualsLiteral("isnot"))
	{
            if (!value.Equals(matchText, nsCaseInsensitiveStringComparator()))
                found = PR_TRUE;
	}
        else if (matchMethod.EqualsLiteral("doesntcontain"))
	{
            if (!FindInReadable(matchText, value,
                                nsCaseInsensitiveStringComparator()))
                found = PR_TRUE;
	}
        return(found);
}

PRBool
LocalSearchDataSource::doDateMatch(nsIRDFDate *aDate,
                                   const nsAString& matchMethod,
                                   const nsAString& matchText)
{
    PRBool found = PR_FALSE;
    
    if (matchMethod.EqualsLiteral("isbefore") ||
        matchMethod.EqualsLiteral("isafter"))
    {
        PRInt64 matchDate;
        nsresult rv = parseDate(matchText, &matchDate);
        if (NS_SUCCEEDED(rv))
            found = dateMatches(aDate, matchMethod, matchDate);
    }

    return found;
}

PRBool
LocalSearchDataSource::doIntMatch(nsIRDFInt *aInt,
                                  const nsAString& matchMethod,
                                  const nsString& matchText)
{
    nsresult rv;
    PRBool found = PR_FALSE;
    
    PRInt32 val;
    rv = aInt->GetValue(&val);
    if (NS_FAILED(rv)) return PR_FALSE;
    
    PRInt32 error=0;
    PRInt32 matchVal = matchText.ToInteger(&error);
    if (error != 0) return PR_FALSE;
    
    if (matchMethod.EqualsLiteral("is"))
        found = (val == matchVal);
    else if (matchMethod.EqualsLiteral("isgreater"))
        found = (val > matchVal);
    else if (matchMethod.EqualsLiteral("isless"))
        found = (val < matchVal);

    return found;
}

NS_METHOD
LocalSearchDataSource::parseDate(const nsAString& aDate,
                                 PRInt64 *aResult)
{
    
    
    PRTime *outTime = NS_STATIC_CAST(PRTime*,aResult);
    PRStatus err;
    err = PR_ParseTimeString(NS_ConvertUTF16toUTF8(aDate).get(),
                             PR_FALSE, 
                             outTime);
    NS_ENSURE_TRUE(err == 0, NS_ERROR_FAILURE);
    
    return NS_OK;
}


PRBool
LocalSearchDataSource::dateMatches(nsIRDFDate *aDate,
                                   const nsAString& method,
                                   const PRInt64& matchDate)
{
    PRInt64 date;
    aDate->GetValue(&date);
    PRBool matches = PR_FALSE;
    
    if (method.EqualsLiteral("isbefore"))
        matches = LL_CMP(date, <, matchDate);
    
    else if (method.EqualsLiteral("isafter"))
        matches = LL_CMP(date, >, matchDate);

    else if (method.EqualsLiteral("is"))
        matches = LL_EQ(date, matchDate);

    return matches;
}


NS_METHOD
LocalSearchDataSource::parseFindURL(nsIRDFResource *u, nsISupportsArray *array)
{
  findTokenStruct		tokens[5];
  nsresult rv;

  
  tokens[0].token = "datasource";
  tokens[1].token = "match";
  tokens[2].token = "method";
  tokens[3].token = "text";
  tokens[4].token = NULL;

  
  
  rv = parseResourceIntoFindTokens(u, tokens);
  if (NS_FAILED(rv)) 
    return rv;

  nsCAutoString dsName;
  dsName.AssignWithConversion(tokens[0].value);

  nsCOMPtr<nsIRDFDataSource> datasource;
  rv = gRDFService->GetDataSource(dsName.get(), getter_AddRefs(datasource));
  if (NS_FAILED(rv)) 
    return rv;

  nsCOMPtr<nsISimpleEnumerator> cursor;
  rv = datasource->GetAllResources(getter_AddRefs(cursor));
  if (NS_FAILED(rv)) 
    return rv;
        
  while (PR_TRUE) {
    PRBool hasMore;
    rv = cursor->HasMoreElements(&hasMore);
    if (NS_FAILED(rv)) 
      break;

    if (!hasMore) 
      break;

    nsCOMPtr<nsISupports> isupports;
    rv = cursor->GetNext(getter_AddRefs(isupports));
    if (NS_FAILED(rv)) 
      continue;

    nsCOMPtr<nsIRDFResource> source(do_QueryInterface(isupports));
    if (!source) 
      continue;

    const char	*uri = nsnull;
    source->GetValueConst(&uri);

    if (!uri) 
      continue;
            
    
    if (PL_strncmp(uri, kFindProtocol, sizeof(kFindProtocol)-1) == 0)
      continue;

    
    
    PRBool isContainer = PR_FALSE;

    
    nsCOMPtr<nsIRDFContainerUtils> cUtils(do_GetService("@mozilla.org/rdf/container-utils;1"));
    if (cUtils)
      cUtils->IsContainer(datasource, source, &isContainer);
    
    if (!isContainer)
      datasource->HasArcOut(source, kNC_Child, &isContainer);

    if (isContainer) 
      continue;

    nsCOMPtr<nsIRDFResource> property;
    rv = gRDFService->GetUnicodeResource(tokens[1].value,
    getter_AddRefs(property));

    if (NS_FAILED(rv) || (rv == NS_RDF_NO_VALUE) || !property)
      continue;

    nsCOMPtr<nsIRDFNode>    value;
    rv = datasource->GetTarget(source, property,
    PR_TRUE, getter_AddRefs(value));
    if (NS_FAILED(rv) || (rv == NS_RDF_NO_VALUE) || !value)
      continue;

    PRBool found = PR_FALSE;
    found = matchNode(value, tokens[2].value, tokens[3].value);

    if (found)
      array->AppendElement(source);
   }

  if (rv == NS_RDF_CURSOR_EMPTY)
    rv = NS_OK;

  return rv;
}




PRBool
LocalSearchDataSource::matchNode(nsIRDFNode *aValue,
                                 const nsAString& matchMethod,
                                 const nsString& matchText)
{
    nsCOMPtr<nsIRDFLiteral> literal(do_QueryInterface(aValue));
    if (literal)
        return doMatch(literal, matchMethod, matchText);

    nsCOMPtr<nsIRDFDate> dateLiteral(do_QueryInterface(aValue));
    if (dateLiteral)
        return doDateMatch(dateLiteral, matchMethod, matchText);
    
    nsCOMPtr<nsIRDFInt> intLiteral(do_QueryInterface(aValue));
    if (intLiteral)
        return doIntMatch(intLiteral, matchMethod, matchText);

    return PR_FALSE;
}

NS_METHOD
LocalSearchDataSource::getFindResults(nsIRDFResource *source, nsISimpleEnumerator** aResult)
{
	nsresult			rv;
	nsCOMPtr<nsISupportsArray>	nameArray;
	rv = NS_NewISupportsArray( getter_AddRefs(nameArray) );
	if (NS_FAILED(rv)) return rv;

	rv = parseFindURL(source, nameArray);
	if (NS_FAILED(rv)) return rv;

        return NS_NewArrayEnumerator(aResult, nameArray);
}



NS_METHOD
LocalSearchDataSource::getFindName(nsIRDFResource *source, nsIRDFLiteral** aResult)
{
	
	*aResult = nsnull;
	return(NS_OK);
}



NS_IMETHODIMP
LocalSearchDataSource::GetTargets(nsIRDFResource *source,
                           nsIRDFResource *property,
                           PRBool tv,
                           nsISimpleEnumerator **targets )
{
	NS_PRECONDITION(source != nsnull, "null ptr");
	if (! source)
		return NS_ERROR_NULL_POINTER;

	NS_PRECONDITION(property != nsnull, "null ptr");
	if (! property)
		return NS_ERROR_NULL_POINTER;

	NS_PRECONDITION(targets != nsnull, "null ptr");
	if (! targets)
		return NS_ERROR_NULL_POINTER;

	nsresult		rv = NS_ERROR_FAILURE;

	
	if (! tv)
		return rv;

	if (isFindURI(source))
	{
		if (property == kNC_Child)
		{
			return getFindResults(source, targets);
		}
		else if (property == kNC_Name)
		{
			nsCOMPtr<nsIRDFLiteral>	name;
			rv = getFindName(source, getter_AddRefs(name));
			if (NS_FAILED(rv)) return rv;

                        return NS_NewSingletonEnumerator(targets, name);
		}
		else if (property == kRDF_type)
		{
			const	char	*uri = nsnull;
			rv = kNC_FindObject->GetValueConst( &uri );
			if (NS_FAILED(rv)) return rv;

			nsAutoString	url; url.AssignWithConversion(uri);

			nsCOMPtr<nsIRDFLiteral> literal;
			rv = gRDFService->GetLiteral(url.get(),
                                                     getter_AddRefs(literal));
			if (NS_FAILED(rv)) return rv;

                        return NS_NewSingletonEnumerator(targets, literal);
		}
		else if (property == kNC_pulse)
		{
			nsAutoString	pulse(NS_LITERAL_STRING("15"));

			nsCOMPtr<nsIRDFLiteral> pulseLiteral;
			rv = gRDFService->GetLiteral(pulse.get(),
                                                     getter_AddRefs(pulseLiteral));
			if (NS_FAILED(rv)) return rv;

                        return NS_NewSingletonEnumerator(targets, pulseLiteral);
		}
	}

	return NS_NewEmptyEnumerator(targets);
}



NS_IMETHODIMP
LocalSearchDataSource::Assert(nsIRDFResource *source,
                       nsIRDFResource *property,
                       nsIRDFNode *target,
                       PRBool tv)
{
	return NS_RDF_ASSERTION_REJECTED;
}



NS_IMETHODIMP
LocalSearchDataSource::Unassert(nsIRDFResource *source,
                         nsIRDFResource *property,
                         nsIRDFNode *target)
{
	return NS_RDF_ASSERTION_REJECTED;
}



NS_IMETHODIMP
LocalSearchDataSource::Change(nsIRDFResource* aSource,
                       nsIRDFResource* aProperty,
                       nsIRDFNode* aOldTarget,
                       nsIRDFNode* aNewTarget)
{
	return NS_RDF_ASSERTION_REJECTED;
}



NS_IMETHODIMP
LocalSearchDataSource::Move(nsIRDFResource* aOldSource,
                     nsIRDFResource* aNewSource,
                     nsIRDFResource* aProperty,
                     nsIRDFNode* aTarget)
{
	return NS_RDF_ASSERTION_REJECTED;
}



NS_IMETHODIMP
LocalSearchDataSource::HasAssertion(nsIRDFResource *source,
                             nsIRDFResource *property,
                             nsIRDFNode *target,
                             PRBool tv,
                             PRBool *hasAssertion )
{
	NS_PRECONDITION(source != nsnull, "null ptr");
	if (! source)
		return NS_ERROR_NULL_POINTER;

	NS_PRECONDITION(property != nsnull, "null ptr");
	if (! property)
		return NS_ERROR_NULL_POINTER;

	NS_PRECONDITION(target != nsnull, "null ptr");
	if (! target)
		return NS_ERROR_NULL_POINTER;

	NS_PRECONDITION(hasAssertion != nsnull, "null ptr");
	if (! hasAssertion)
		return NS_ERROR_NULL_POINTER;

	nsresult		rv = NS_OK;

	*hasAssertion = PR_FALSE;

	
	if (! tv)
		return rv;

	if (isFindURI(source))
	{
		if (property == kRDF_type)
		{
			if ((nsIRDFResource *)target == kRDF_type)
			{
				*hasAssertion = PR_TRUE;
			}
		}
	}
	return (rv);
}

NS_IMETHODIMP 
LocalSearchDataSource::HasArcIn(nsIRDFNode *aNode, nsIRDFResource *aArc, PRBool *result)
{
    *result = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP 
LocalSearchDataSource::HasArcOut(nsIRDFResource *source, nsIRDFResource *aArc, PRBool *result)
{
    NS_PRECONDITION(source != nsnull, "null ptr");
    if (! source)
        return NS_ERROR_NULL_POINTER;

    if ((aArc == kNC_Child ||
         aArc == kNC_pulse)) {
        *result = isFindURI(source);
    }
    else {
        *result = PR_FALSE;
    }
    return NS_OK;
}

NS_IMETHODIMP
LocalSearchDataSource::ArcLabelsIn(nsIRDFNode *node,
                            nsISimpleEnumerator ** labels )
{
	NS_NOTYETIMPLEMENTED("write me");
	return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
LocalSearchDataSource::ArcLabelsOut(nsIRDFResource *source,
                             nsISimpleEnumerator **labels )
{
	NS_PRECONDITION(source != nsnull, "null ptr");
	if (! source)
		return NS_ERROR_NULL_POINTER;

	NS_PRECONDITION(labels != nsnull, "null ptr");
	if (! labels)
		return NS_ERROR_NULL_POINTER;

	nsresult		rv;

	if (isFindURI(source))
	{
		nsCOMPtr<nsISupportsArray> array;
		rv = NS_NewISupportsArray( getter_AddRefs(array) );
		if (NS_FAILED(rv)) return rv;

		array->AppendElement(kNC_Child);
		array->AppendElement(kNC_pulse);

                return NS_NewArrayEnumerator(labels, array);
	}
	return(NS_NewEmptyEnumerator(labels));
}



NS_IMETHODIMP
LocalSearchDataSource::GetAllResources(nsISimpleEnumerator** aCursor)
{
	NS_NOTYETIMPLEMENTED("sorry!");
	return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
LocalSearchDataSource::AddObserver(nsIRDFObserver *n)
{
	NS_PRECONDITION(n != nsnull, "null ptr");
	if (! n)
		return NS_ERROR_NULL_POINTER;

	if (! mObservers)
	{
		nsresult	rv;
		rv = NS_NewISupportsArray(getter_AddRefs(mObservers));
		if (NS_FAILED(rv)) return rv;
	}
	return mObservers->AppendElement(n) ? NS_OK : NS_ERROR_FAILURE;
}



NS_IMETHODIMP
LocalSearchDataSource::RemoveObserver(nsIRDFObserver *n)
{
	NS_PRECONDITION(n != nsnull, "null ptr");
	if (! n)
		return NS_ERROR_NULL_POINTER;

	if (! mObservers)
		return(NS_OK);

#ifdef DEBUG
	PRBool ok =
#endif
	mObservers->RemoveElement(n);

	NS_ASSERTION(ok, "observer not present");

	return(NS_OK);
}



NS_IMETHODIMP
LocalSearchDataSource::GetAllCmds(nsIRDFResource* source, nsISimpleEnumerator** commands)
{
	return(NS_NewEmptyEnumerator(commands));
}



NS_IMETHODIMP
LocalSearchDataSource::IsCommandEnabled(nsISupportsArray* aSources,
				nsIRDFResource*   aCommand,
				nsISupportsArray* aArguments,
                                PRBool* aResult)
{
	return(NS_ERROR_NOT_IMPLEMENTED);
}



NS_IMETHODIMP
LocalSearchDataSource::DoCommand(nsISupportsArray* aSources,
				nsIRDFResource*   aCommand,
				nsISupportsArray* aArguments)
{
	return(NS_ERROR_NOT_IMPLEMENTED);
}



NS_IMETHODIMP
LocalSearchDataSource::BeginUpdateBatch()
{
	return NS_OK;
}



NS_IMETHODIMP
LocalSearchDataSource::EndUpdateBatch()
{
	return NS_OK;
}
