



















#include "nsStreamConverterService.h"
#include "nsIComponentRegistrar.h"
#include "nsString.h"
#include "nsIAtom.h"
#include "nsDeque.h"
#include "nsIInputStream.h"
#include "nsIStreamConverter.h"
#include "nsICategoryManager.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsCOMArray.h"
#include "nsTArray.h"
#include "nsServiceManagerUtils.h"
#include "nsHashtable.h"
#include "nsISimpleEnumerator.h"





enum BFScolors {white, gray, black};

struct BFSState {
    BFScolors   color;
    int32_t     distance;
    nsCStringKey  *predecessor;
    ~BFSState() {
        delete predecessor;
    }
};


typedef nsCOMArray<nsIAtom> SCTableData;


static bool DeleteAdjacencyEntry(nsHashKey *aKey, void *aData, void* closure) {
    SCTableData *entry = (SCTableData*)aData;
    delete entry;
    return true;
}


struct BFSTableData {
    nsCStringKey *key;
    union _data {
        BFSState *state;
        nsCOMArray<nsIAtom> *edges;
    } data;

    BFSTableData(nsCStringKey* aKey) : key(aKey) {
        data.state = nullptr;
    }
};



NS_IMPL_ISUPPORTS1(nsStreamConverterService, nsIStreamConverterService)







nsStreamConverterService::nsStreamConverterService()
  : mAdjacencyList(nullptr, nullptr, DeleteAdjacencyEntry, nullptr)
{
}

nsStreamConverterService::~nsStreamConverterService() {
}















nsresult
nsStreamConverterService::BuildGraph() {

    nsresult rv;

    nsCOMPtr<nsICategoryManager> catmgr(do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsISimpleEnumerator> entries;
    rv = catmgr->EnumerateCategory(NS_ISTREAMCONVERTER_KEY, getter_AddRefs(entries));
    if (NS_FAILED(rv)) return rv;

    
    nsCOMPtr<nsISupports> supports;
    nsCOMPtr<nsISupportsCString> entry;
    rv = entries->GetNext(getter_AddRefs(supports));
    while (NS_SUCCEEDED(rv)) {
        entry = do_QueryInterface(supports);

        
        nsAutoCString entryString;
        rv = entry->GetData(entryString);
        if (NS_FAILED(rv)) return rv;

        
        nsAutoCString contractID(NS_ISTREAMCONVERTER_KEY);
        contractID.Append(entryString);

        
        rv = AddAdjacency(contractID.get());
        if (NS_FAILED(rv)) return rv;

        rv = entries->GetNext(getter_AddRefs(supports));
    }

    return NS_OK;
}






nsresult
nsStreamConverterService::AddAdjacency(const char *aContractID) {
    nsresult rv;
    

    nsAutoCString fromStr, toStr;
    rv = ParseFromTo(aContractID, fromStr, toStr);
    if (NS_FAILED(rv)) return rv;

    
    

    nsCStringKey fromKey(fromStr);
    SCTableData *fromEdges = (SCTableData*)mAdjacencyList.Get(&fromKey);
    if (!fromEdges) {
        

        nsCStringKey *newFromKey = new nsCStringKey(ToNewCString(fromStr), fromStr.Length(), nsCStringKey::OWN);
        SCTableData *data = new SCTableData();
        mAdjacencyList.Put(newFromKey, data);
        fromEdges = data;
    }

    nsCStringKey toKey(toStr);
    if (!mAdjacencyList.Get(&toKey)) {
        
        nsCStringKey *newToKey = new nsCStringKey(ToNewCString(toStr), toStr.Length(), nsCStringKey::OWN);
        SCTableData *data = new SCTableData();
        mAdjacencyList.Put(newToKey, data);
    }

    
    

    nsCOMPtr<nsIAtom> vertex = do_GetAtom(toStr); 
    if (!vertex) return NS_ERROR_OUT_OF_MEMORY;

    NS_ASSERTION(fromEdges, "something wrong in adjacency list construction");
    if (!fromEdges)
        return NS_ERROR_FAILURE;

    return fromEdges->AppendObject(vertex) ? NS_OK : NS_ERROR_FAILURE;
}

nsresult
nsStreamConverterService::ParseFromTo(const char *aContractID, nsCString &aFromRes, nsCString &aToRes) {

    nsAutoCString ContractIDStr(aContractID);

    int32_t fromLoc = ContractIDStr.Find("from=");
    int32_t toLoc   = ContractIDStr.Find("to=");
    if (-1 == fromLoc || -1 == toLoc ) return NS_ERROR_FAILURE;

    fromLoc = fromLoc + 5;
    toLoc = toLoc + 3;

    nsAutoCString fromStr, toStr;

    ContractIDStr.Mid(fromStr, fromLoc, toLoc - 4 - fromLoc);
    ContractIDStr.Mid(toStr, toLoc, ContractIDStr.Length() - toLoc);

    aFromRes.Assign(fromStr);
    aToRes.Assign(toStr);

    return NS_OK;
}




static bool InitBFSTable(nsHashKey *aKey, void *aData, void* closure) {
    NS_ASSERTION((SCTableData*)aData, "no data in the table enumeration");

    nsHashtable *BFSTable = (nsHashtable*)closure;
    if (!BFSTable) return false;

    BFSState *state = new BFSState;

    state->color = white;
    state->distance = -1;
    state->predecessor = nullptr;

    BFSTableData *data = new BFSTableData(static_cast<nsCStringKey*>(aKey));
    data->data.state = state;

    BFSTable->Put(aKey, data);
    return true;
}


static bool DeleteBFSEntry(nsHashKey *aKey, void *aData, void *closure) {
    BFSTableData *data = (BFSTableData*)aData;
    BFSState *state = data->data.state;
    delete state;
    data->key = nullptr;
    delete data;
    return true;
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
nsStreamConverterService::FindConverter(const char *aContractID, nsTArray<nsCString> **aEdgeList) {
    nsresult rv;
    if (!aEdgeList) return NS_ERROR_NULL_POINTER;
    *aEdgeList = nullptr;

    

    int32_t vertexCount = mAdjacencyList.Count();
    if (0 >= vertexCount) return NS_ERROR_FAILURE;

    
    nsObjectHashtable lBFSTable(nullptr, nullptr, DeleteBFSEntry, nullptr);
    mAdjacencyList.Enumerate(InitBFSTable, &lBFSTable);

    NS_ASSERTION(lBFSTable.Count() == vertexCount, "strmconv BFS table init problem");

    
    nsAutoCString fromC, toC;
    rv = ParseFromTo(aContractID, fromC, toC);
    if (NS_FAILED(rv)) return rv;

    nsCStringKey *source = new nsCStringKey(fromC.get());

    BFSTableData *data = (BFSTableData*)lBFSTable.Get(source);
    if (!data) {
        delete source;
        return NS_ERROR_FAILURE;
    }

    BFSState *state = data->data.state;

    state->color = gray;
    state->distance = 0;
    CStreamConvDeallocator *dtorFunc = new CStreamConvDeallocator();

    nsDeque grayQ(dtorFunc);

    
    grayQ.Push(source);
    while (0 < grayQ.GetSize()) {
        nsCStringKey *currentHead = (nsCStringKey*)grayQ.PeekFront();
        SCTableData *data2 = (SCTableData*)mAdjacencyList.Get(currentHead);
        if (!data2) return NS_ERROR_FAILURE;

        
        
        BFSTableData *data2b = (BFSTableData*)lBFSTable.Get(currentHead);
        if (!data2b) return NS_ERROR_FAILURE;

        BFSState *headVertexState = data2b->data.state;
        NS_ASSERTION(headVertexState, "problem with the BFS strmconv algorithm");
        if (!headVertexState) return NS_ERROR_FAILURE;

        int32_t edgeCount = data2->Count();

        for (int32_t i = 0; i < edgeCount; i++) {
            nsIAtom* curVertexAtom = data2->ObjectAt(i);
            nsAutoString curVertexStr;
            curVertexAtom->ToString(curVertexStr);
            nsCStringKey *curVertex = new nsCStringKey(ToNewCString(curVertexStr), 
                                        curVertexStr.Length(), nsCStringKey::OWN);

            BFSTableData *data3 = (BFSTableData*)lBFSTable.Get(curVertex);
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
        cur = nullptr;
    }
    
    

    

    nsAutoCString fromStr, toStr;
    rv = ParseFromTo(aContractID, fromStr, toStr);
    if (NS_FAILED(rv)) return rv;

    
    nsAutoCString ContractIDPrefix(NS_ISTREAMCONVERTER_KEY);
    nsTArray<nsCString> *shortestPath = new nsTArray<nsCString>();

    nsCStringKey toMIMEType(toStr);
    data = (BFSTableData*)lBFSTable.Get(&toMIMEType);
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
        BFSTableData *predecessorData = (BFSTableData*)lBFSTable.Get(curState->predecessor);

        if (!predecessorData) break; 

        
        nsAutoCString newContractID(ContractIDPrefix);
        newContractID.AppendLiteral("?from=");

        nsCStringKey *predecessorKey = predecessorData->key;
        newContractID.Append(predecessorKey->GetString());

        newContractID.AppendLiteral("&to=");
        newContractID.Append(key->GetString());

        
        rv = shortestPath->AppendElement(newContractID) ? NS_OK : NS_ERROR_FAILURE;  
        NS_ASSERTION(NS_SUCCEEDED(rv), "AppendElement failed");

        
        data = predecessorData;
    }
    delete shortestPath;
    return NS_ERROR_FAILURE; 
}




NS_IMETHODIMP
nsStreamConverterService::CanConvert(const char* aFromType,
                                     const char* aToType,
                                     bool* _retval) {
    nsCOMPtr<nsIComponentRegistrar> reg;
    nsresult rv = NS_GetComponentRegistrar(getter_AddRefs(reg));
    if (NS_FAILED(rv))
        return rv;

    nsAutoCString contractID;
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

    nsTArray<nsCString> *converterChain = nullptr;
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

    
    
    nsAutoCString contractID;
    contractID.AssignLiteral(NS_ISTREAMCONVERTER_KEY "?from=");
    contractID.Append(aFromType);
    contractID.AppendLiteral("&to=");
    contractID.Append(aToType);
    const char *cContractID = contractID.get();

    nsCOMPtr<nsIStreamConverter> converter(do_CreateInstance(cContractID, &rv));
    if (NS_FAILED(rv)) {
        
        rv = BuildGraph();
        if (NS_FAILED(rv)) return rv;

        nsTArray<nsCString> *converterChain = nullptr;

        rv = FindConverter(cContractID, &converterChain);
        if (NS_FAILED(rv)) {
            
            
            return NS_ERROR_FAILURE;
        }

        int32_t edgeCount = int32_t(converterChain->Length());
        NS_ASSERTION(edgeCount > 0, "findConverter should have failed");


        
        
        nsCOMPtr<nsIInputStream> dataToConvert = aFromStream;
        nsCOMPtr<nsIInputStream> convertedData;

        for (int32_t i = edgeCount-1; i >= 0; i--) {
            const char *lContractID = converterChain->ElementAt(i).get();

            converter = do_CreateInstance(lContractID, &rv);

            if (NS_FAILED(rv)) {
                delete converterChain;
                return rv;
            }

            nsAutoCString fromStr, toStr;
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

    
    
    nsAutoCString contractID;
    contractID.AssignLiteral(NS_ISTREAMCONVERTER_KEY "?from=");
    contractID.Append(aFromType);
    contractID.AppendLiteral("&to=");
    contractID.Append(aToType);
    const char *cContractID = contractID.get();

    nsCOMPtr<nsIStreamConverter> listener(do_CreateInstance(cContractID, &rv));
    if (NS_FAILED(rv)) {
        
        rv = BuildGraph();
        if (NS_FAILED(rv)) return rv;

        nsTArray<nsCString> *converterChain = nullptr;

        rv = FindConverter(cContractID, &converterChain);
        if (NS_FAILED(rv)) {
            
            
            return NS_ERROR_FAILURE;
        }

        
        
        
        
        nsCOMPtr<nsIStreamListener> finalListener = aListener;

        
        
        int32_t edgeCount = int32_t(converterChain->Length());
        NS_ASSERTION(edgeCount > 0, "findConverter should have failed");
        for (int i = 0; i < edgeCount; i++) {
            const char *lContractID = converterChain->ElementAt(i).get();

            
            nsCOMPtr<nsIStreamConverter> converter(do_CreateInstance(lContractID));
            NS_ASSERTION(converter, "graph construction problem, built a contractid that wasn't registered");

            nsAutoCString fromStr, toStr;
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
    NS_PRECONDITION(aStreamConv != nullptr, "null ptr");
    if (!aStreamConv) return NS_ERROR_NULL_POINTER;

    *aStreamConv = new nsStreamConverterService();
    NS_ADDREF(*aStreamConv);

    return NS_OK;
}
