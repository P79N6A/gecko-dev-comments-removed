



















































#include "nsStreamConverterService.h"
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsIComponentRegistrar.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsIAtom.h"
#include "nsDeque.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsIStreamConverter.h"
#include "nsICategoryManager.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsXPIDLString.h"



NS_IMPL_THREADSAFE_ISUPPORTS1(nsStreamConverterService, nsIStreamConverterService)







nsStreamConverterService::nsStreamConverterService() : mAdjacencyList(nsnull) {
}

nsStreamConverterService::~nsStreamConverterService() {
    NS_ASSERTION(mAdjacencyList, "init wasn't called, or the retval was ignored");
    delete mAdjacencyList;
}


static PRBool PR_CALLBACK DeleteAdjacencyEntry(nsHashKey *aKey, void *aData, void* closure) {
    SCTableData *entry = (SCTableData*)aData;
    NS_ASSERTION(entry->key && entry->data.edges, "malformed adjacency list entry");
    delete entry->key;
    delete entry->data.edges;
    delete entry;
    return PR_TRUE;   
}

nsresult
nsStreamConverterService::Init() {
    mAdjacencyList = new nsObjectHashtable(nsnull, nsnull,
                                           DeleteAdjacencyEntry, nsnull);
    if (!mAdjacencyList) return NS_ERROR_OUT_OF_MEMORY;
    return NS_OK;
}















nsresult
nsStreamConverterService::BuildGraph() {

    nsresult rv;

    nsCOMPtr<nsICategoryManager> catmgr(do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsISimpleEnumerator> entries;
    rv = catmgr->EnumerateCategory(NS_ISTREAMCONVERTER_KEY, getter_AddRefs(entries));
    if (NS_FAILED(rv)) return rv;

    
    nsCOMPtr<nsISupportsCString> entry;
    rv = entries->GetNext(getter_AddRefs(entry));
    while (NS_SUCCEEDED(rv)) {

        
        nsCAutoString entryString;
        rv = entry->GetData(entryString);
        if (NS_FAILED(rv)) return rv;
        
        
        nsCAutoString contractID(NS_ISTREAMCONVERTER_KEY);
        contractID.Append(entryString);

        
        rv = AddAdjacency(contractID.get());
        if (NS_FAILED(rv)) return rv;

        rv = entries->GetNext(getter_AddRefs(entry));
    }

    return NS_OK;
}






nsresult
nsStreamConverterService::AddAdjacency(const char *aContractID) {
    nsresult rv;
    

    nsCAutoString fromStr, toStr;
    rv = ParseFromTo(aContractID, fromStr, toStr);
    if (NS_FAILED(rv)) return rv;

    
    

    nsCStringKey fromKey(fromStr);
    SCTableData *fromEdges = (SCTableData*)mAdjacencyList->Get(&fromKey);
    if (!fromEdges) {
        

        nsCStringKey *newFromKey = new nsCStringKey(ToNewCString(fromStr), fromStr.Length(), nsCStringKey::OWN);
        if (!newFromKey) return NS_ERROR_OUT_OF_MEMORY;

        SCTableData *data = new SCTableData(newFromKey);
        if (!data) {
            delete newFromKey;
            return NS_ERROR_OUT_OF_MEMORY;
        }

        nsCOMArray<nsIAtom>* edgeArray = new nsCOMArray<nsIAtom>;
        if (!edgeArray) {
            delete newFromKey;
            data->key = nsnull;
            delete data;
            return NS_ERROR_OUT_OF_MEMORY;
        }
        data->data.edges = edgeArray;

        mAdjacencyList->Put(newFromKey, data);
        fromEdges = data;
    }

    nsCStringKey toKey(toStr);
    if (!mAdjacencyList->Get(&toKey)) {
        
        nsCStringKey *newToKey = new nsCStringKey(ToNewCString(toStr), toStr.Length(), nsCStringKey::OWN);
        if (!newToKey) return NS_ERROR_OUT_OF_MEMORY;

        SCTableData *data = new SCTableData(newToKey);
        if (!data) {
            delete newToKey;
            return NS_ERROR_OUT_OF_MEMORY;
        }

        nsCOMArray<nsIAtom>* edgeArray = new nsCOMArray<nsIAtom>;
        if (!edgeArray) {
            delete newToKey;
            data->key = nsnull;
            delete data;
            return NS_ERROR_OUT_OF_MEMORY;
        }
        data->data.edges = edgeArray;
        mAdjacencyList->Put(newToKey, data);
    }
    
    
    

    nsCOMPtr<nsIAtom> vertex = do_GetAtom(toStr.get()); 
    if (!vertex) return NS_ERROR_OUT_OF_MEMORY;

    NS_ASSERTION(fromEdges, "something wrong in adjacency list construction");
    if (!fromEdges)
        return NS_ERROR_FAILURE;

    nsCOMArray<nsIAtom> *adjacencyList = fromEdges->data.edges;
    return adjacencyList->AppendObject(vertex) ? NS_OK : NS_ERROR_FAILURE;
}

nsresult
nsStreamConverterService::ParseFromTo(const char *aContractID, nsCString &aFromRes, nsCString &aToRes) {

    nsCAutoString ContractIDStr(aContractID);

    PRInt32 fromLoc = ContractIDStr.Find("from=");
    PRInt32 toLoc   = ContractIDStr.Find("to=");
    if (-1 == fromLoc || -1 == toLoc ) return NS_ERROR_FAILURE;

    fromLoc = fromLoc + 5;
    toLoc = toLoc + 3;

    nsCAutoString fromStr, toStr;

    ContractIDStr.Mid(fromStr, fromLoc, toLoc - 4 - fromLoc);
    ContractIDStr.Mid(toStr, toLoc, ContractIDStr.Length() - toLoc);

    aFromRes.Assign(fromStr);
    aToRes.Assign(toStr);

    return NS_OK;
}




static PRBool PR_CALLBACK InitBFSTable(nsHashKey *aKey, void *aData, void* closure) {
    NS_ASSERTION((SCTableData*)aData, "no data in the table enumeration");
    
    nsHashtable *BFSTable = (nsHashtable*)closure;
    if (!BFSTable) return PR_FALSE;

    BFSState *state = new BFSState;
    if (!state) return PR_FALSE;

    state->color = white;
    state->distance = -1;
    state->predecessor = nsnull;

    SCTableData *data = new SCTableData(NS_STATIC_CAST(nsCStringKey*, aKey));
    if (!data) {
        delete state;
        return PR_FALSE;
    }
    data->data.state = state;

    BFSTable->Put(aKey, data);
    return PR_TRUE;   
}


static PRBool PR_CALLBACK DeleteBFSEntry(nsHashKey *aKey, void *aData, void *closure) {
    SCTableData *data = (SCTableData*)aData;
    BFSState *state = data->data.state;
    delete state;
    data->key = nsnull;
    delete data;
    return PR_TRUE;
}

class CStreamConvDeallocator : public nsDequeFunctor {
public:
    virtual void* operator()(void* anObject) {
        nsCStringKey *key = (nsCStringKey*)anObject;
        delete key;
        return 0;
    }
};






nsresult
nsStreamConverterService::FindConverter(const char *aContractID, nsCStringArray **aEdgeList) {
    nsresult rv;
    if (!aEdgeList) return NS_ERROR_NULL_POINTER;
    *aEdgeList = nsnull;

    

    PRInt32 vertexCount = mAdjacencyList->Count();
    if (0 >= vertexCount) return NS_ERROR_FAILURE;

    
    nsObjectHashtable lBFSTable(nsnull, nsnull, DeleteBFSEntry, nsnull);
    mAdjacencyList->Enumerate(InitBFSTable, &lBFSTable);

    NS_ASSERTION(lBFSTable.Count() == vertexCount, "strmconv BFS table init problem");

    
    nsCAutoString fromC, toC;
    rv = ParseFromTo(aContractID, fromC, toC);
    if (NS_FAILED(rv)) return rv;

    nsCStringKey *source = new nsCStringKey(fromC.get());
    if (!source) return NS_ERROR_OUT_OF_MEMORY;

    SCTableData *data = (SCTableData*)lBFSTable.Get(source);
    if (!data) {
        delete source;
        return NS_ERROR_FAILURE;
    }

    BFSState *state = data->data.state;

    state->color = gray;
    state->distance = 0;
    CStreamConvDeallocator *dtorFunc = new CStreamConvDeallocator();
    if (!dtorFunc) {
        delete source;
        return NS_ERROR_OUT_OF_MEMORY;
    }

    nsDeque grayQ(dtorFunc);

    
    grayQ.Push(source);
    while (0 < grayQ.GetSize()) {
        nsCStringKey *currentHead = (nsCStringKey*)grayQ.PeekFront();
        SCTableData *data2 = (SCTableData*)mAdjacencyList->Get(currentHead);
        if (!data2) return NS_ERROR_FAILURE;

        nsCOMArray<nsIAtom> *edges = data2->data.edges;
        NS_ASSERTION(edges, "something went wrong with BFS strmconv algorithm");
        if (!edges) return NS_ERROR_FAILURE;

        
        
        data2 = (SCTableData*)lBFSTable.Get(currentHead);
        if (!data2) return NS_ERROR_FAILURE;

        BFSState *headVertexState = data2->data.state;
        NS_ASSERTION(headVertexState, "problem with the BFS strmconv algorithm");
        if (!headVertexState) return NS_ERROR_FAILURE;

        PRInt32 edgeCount = edges->Count();

        for (PRInt32 i = 0; i < edgeCount; i++) {
            nsIAtom* curVertexAtom = edges->ObjectAt(i);
            nsAutoString curVertexStr;
            curVertexAtom->ToString(curVertexStr);
            nsCStringKey *curVertex = new nsCStringKey(ToNewCString(curVertexStr), 
                                        curVertexStr.Length(), nsCStringKey::OWN);
            if (!curVertex) return NS_ERROR_OUT_OF_MEMORY;

            SCTableData *data3 = (SCTableData*)lBFSTable.Get(curVertex);
            if (!data3) {
                delete curVertex;
                return NS_ERROR_FAILURE;
            }
            BFSState *curVertexState = data3->data.state;
            NS_ASSERTION(curVertexState, "something went wrong with the BFS strmconv algorithm");
            if (!curVertexState) return NS_ERROR_FAILURE;

            if (white == curVertexState->color) {
                curVertexState->color = gray;
                curVertexState->distance = headVertexState->distance + 1;
                curVertexState->predecessor = (nsCStringKey*)currentHead->Clone();
                if (!curVertexState->predecessor) {
                    delete curVertex;
                    return NS_ERROR_OUT_OF_MEMORY;
                }
                grayQ.Push(curVertex);
            } else {
                delete curVertex; 
                                  
                                  
            }
        }
        headVertexState->color = black;
        nsCStringKey *cur = (nsCStringKey*)grayQ.PopFront();
        delete cur;
        cur = nsnull;
    }
    
    

    

    nsCAutoString fromStr, toStr;
    rv = ParseFromTo(aContractID, fromStr, toStr);
    if (NS_FAILED(rv)) return rv;

    
    nsCAutoString ContractIDPrefix(NS_ISTREAMCONVERTER_KEY);
    nsCStringArray *shortestPath = new nsCStringArray();
    if (!shortestPath) return NS_ERROR_OUT_OF_MEMORY;

    nsCStringKey toMIMEType(toStr);
    data = (SCTableData*)lBFSTable.Get(&toMIMEType);
    if (!data) {
        
        
        delete shortestPath;
        return NS_ERROR_FAILURE;
    }

    while (data) {
        BFSState *curState = data->data.state;

        nsCStringKey *key = data->key;
        
        if (fromStr.Equals(key->GetString())) {
            
            *aEdgeList = shortestPath;
            return NS_OK;
        }

        
        
        if (!curState->predecessor) break; 
        SCTableData *predecessorData = (SCTableData*)lBFSTable.Get(curState->predecessor);

        if (!predecessorData) break; 

        
        nsCAutoString newContractID(ContractIDPrefix);
        newContractID.AppendLiteral("?from=");

        nsCStringKey *predecessorKey = predecessorData->key;
        newContractID.Append(predecessorKey->GetString());

        newContractID.AppendLiteral("&to=");
        newContractID.Append(key->GetString());
    
        
        rv = shortestPath->AppendCString(newContractID) ? NS_OK : NS_ERROR_FAILURE;  
        NS_ASSERTION(NS_SUCCEEDED(rv), "AppendElement failed");

        
        data = predecessorData;
    }
    delete shortestPath;
    return NS_ERROR_FAILURE; 
}




NS_IMETHODIMP
nsStreamConverterService::CanConvert(const char* aFromType,
                                     const char* aToType,
                                     PRBool* _retval) {
    nsCOMPtr<nsIComponentRegistrar> reg;
    nsresult rv = NS_GetComponentRegistrar(getter_AddRefs(reg));
    if (NS_FAILED(rv))
        return rv;

    nsCAutoString contractID;
    contractID.AssignLiteral(NS_ISTREAMCONVERTER_KEY "?from=");
    contractID.Append(aFromType);
    contractID.AppendLiteral("&to=");
    contractID.Append(aToType);

    
    rv = reg->IsContractIDRegistered(contractID.get(), _retval);
    if (NS_FAILED(rv))
        return rv;
    if (*_retval)
        return NS_OK;

    
    rv = BuildGraph();
    if (NS_FAILED(rv))
        return rv;

    nsCStringArray *converterChain = nsnull;
    rv = FindConverter(contractID.get(), &converterChain);
    *_retval = NS_SUCCEEDED(rv);

    delete converterChain;
    return NS_OK;
}

NS_IMETHODIMP
nsStreamConverterService::Convert(nsIInputStream *aFromStream,
                                  const char *aFromType, 
                                  const char *aToType,
                                  nsISupports *aContext,
                                  nsIInputStream **_retval) {
    if (!aFromStream || !aFromType || !aToType || !_retval) return NS_ERROR_NULL_POINTER;
    nsresult rv;

    
    
    nsCAutoString contractID;
    contractID.AssignLiteral(NS_ISTREAMCONVERTER_KEY "?from=");
    contractID.Append(aFromType);
    contractID.AppendLiteral("&to=");
    contractID.Append(aToType);
    const char *cContractID = contractID.get();

    nsCOMPtr<nsIStreamConverter> converter(do_CreateInstance(cContractID, &rv));
    if (NS_FAILED(rv)) {
        
        rv = BuildGraph();
        if (NS_FAILED(rv)) return rv;

        nsCStringArray *converterChain = nsnull;

        rv = FindConverter(cContractID, &converterChain);
        if (NS_FAILED(rv)) {
            
            
            return NS_ERROR_FAILURE;
        }

        PRInt32 edgeCount = converterChain->Count();
        NS_ASSERTION(edgeCount > 0, "findConverter should have failed");


        
        
        nsCOMPtr<nsIInputStream> dataToConvert = aFromStream;
        nsCOMPtr<nsIInputStream> convertedData;

        for (PRInt32 i = edgeCount-1; i >= 0; i--) {
            nsCString *contractIDStr = converterChain->CStringAt(i);
            if (!contractIDStr) {
                delete converterChain;
                return NS_ERROR_FAILURE;
            }
            const char *lContractID = contractIDStr->get();

            converter = do_CreateInstance(lContractID, &rv);

            if (NS_FAILED(rv)) {
                delete converterChain;                
                return rv;
            }

            nsCAutoString fromStr, toStr;
            rv = ParseFromTo(lContractID, fromStr, toStr);
            if (NS_FAILED(rv)) {
                delete converterChain;
                return rv;
            }

            rv = converter->Convert(dataToConvert, fromStr.get(), toStr.get(), aContext, getter_AddRefs(convertedData));
            dataToConvert = convertedData;
            if (NS_FAILED(rv)) {
                delete converterChain;
                return rv;
            }
        }

        delete converterChain;
        *_retval = convertedData;
        NS_ADDREF(*_retval);

    } else {
        
        rv = converter->Convert(aFromStream, aFromType, aToType, aContext, _retval);
    }
    
    return rv;
}


NS_IMETHODIMP
nsStreamConverterService::AsyncConvertData(const char *aFromType, 
                                           const char *aToType, 
                                           nsIStreamListener *aListener,
                                           nsISupports *aContext,
                                           nsIStreamListener **_retval) {
    if (!aFromType || !aToType || !aListener || !_retval) return NS_ERROR_NULL_POINTER;

    nsresult rv;

    
    
    nsCAutoString contractID;
    contractID.AssignLiteral(NS_ISTREAMCONVERTER_KEY "?from=");
    contractID.Append(aFromType);
    contractID.AppendLiteral("&to=");
    contractID.Append(aToType);
    const char *cContractID = contractID.get();

    nsCOMPtr<nsIStreamConverter> listener(do_CreateInstance(cContractID, &rv));
    if (NS_FAILED(rv)) {
        
        rv = BuildGraph();
        if (NS_FAILED(rv)) return rv;

        nsCStringArray *converterChain = nsnull;

        rv = FindConverter(cContractID, &converterChain);
        if (NS_FAILED(rv)) {
            
            
            return NS_ERROR_FAILURE;
        }

        
        
        
        
        nsCOMPtr<nsIStreamListener> finalListener = aListener;

        
        
        PRInt32 edgeCount = converterChain->Count();
        NS_ASSERTION(edgeCount > 0, "findConverter should have failed");
        for (int i = 0; i < edgeCount; i++) {
            nsCString *contractIDStr = converterChain->CStringAt(i);
            if (!contractIDStr) {
                delete converterChain;
                return NS_ERROR_FAILURE;
            }
            const char *lContractID = contractIDStr->get();

            
            nsCOMPtr<nsIStreamConverter> converter(do_CreateInstance(lContractID, &rv));
            NS_ASSERTION(NS_SUCCEEDED(rv), "graph construction problem, built a contractid that wasn't registered");

            nsCAutoString fromStr, toStr;
            rv = ParseFromTo(lContractID, fromStr, toStr);
            if (NS_FAILED(rv)) {
                delete converterChain;
                return rv;
            }

            
            rv = converter->AsyncConvertData(fromStr.get(), toStr.get(), finalListener, aContext);
            if (NS_FAILED(rv)) {
                delete converterChain;
                return rv;
            }

            nsCOMPtr<nsIStreamListener> chainListener(do_QueryInterface(converter, &rv));
            if (NS_FAILED(rv)) {
                delete converterChain;
                return rv;
            }

            
            
            
            
            
            finalListener = chainListener;
        }
        delete converterChain;
        
        *_retval = finalListener;
        NS_ADDREF(*_retval);

    } else {
        
        *_retval = listener;
        NS_ADDREF(*_retval);

        rv = listener->AsyncConvertData(aFromType, aToType, aListener, aContext);
    }
    
    return rv;

}

nsresult
NS_NewStreamConv(nsStreamConverterService** aStreamConv)
{
    NS_PRECONDITION(aStreamConv != nsnull, "null ptr");
    if (!aStreamConv) return NS_ERROR_NULL_POINTER;

    *aStreamConv = new nsStreamConverterService();
    if (!*aStreamConv) return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*aStreamConv);
    nsresult rv = (*aStreamConv)->Init();
    if (NS_FAILED(rv))
        NS_RELEASE(*aStreamConv);

    return rv;
}
