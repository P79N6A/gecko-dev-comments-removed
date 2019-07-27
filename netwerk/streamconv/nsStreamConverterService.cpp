



















#include "nsStreamConverterService.h"
#include "nsIComponentRegistrar.h"
#include "nsAutoPtr.h"
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
#include "nsISimpleEnumerator.h"





enum BFScolors {white, gray, black};


struct BFSTableData {
    nsCString key;
    BFScolors color;
    int32_t distance;
    nsAutoPtr<nsCString> predecessor;

    explicit BFSTableData(const nsACString& aKey)
      : key(aKey), color(white), distance(-1)
    {
    }
};



NS_IMPL_ISUPPORTS(nsStreamConverterService, nsIStreamConverterService)







nsStreamConverterService::nsStreamConverterService()
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

    
    

    nsCOMArray<nsIAtom> *fromEdges = mAdjacencyList.Get(fromStr);
    if (!fromEdges) {
        
        fromEdges = new nsCOMArray<nsIAtom>();
        mAdjacencyList.Put(fromStr, fromEdges);
    }

    if (!mAdjacencyList.Get(toStr)) {
        
        mAdjacencyList.Put(toStr, new nsCOMArray<nsIAtom>());
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

typedef nsClassHashtable<nsCStringHashKey, BFSTableData> BFSHashTable;





static PLDHashOperator
InitBFSTable(const nsACString &aKey, nsCOMArray<nsIAtom> *aData, void* aClosure) {
    MOZ_ASSERT(aData, "no data in the table enumeration");

    BFSHashTable *bfsTable = static_cast<BFSHashTable*>(aClosure);
    if (!bfsTable) return PL_DHASH_STOP;

    BFSTableData *data = new BFSTableData(aKey);
    bfsTable->Put(aKey, data);
    return PL_DHASH_NEXT;
}

class CStreamConvDeallocator : public nsDequeFunctor {
public:
    virtual void* operator()(void* anObject) {
        nsCString *string = (nsCString*)anObject;
        delete string;
        return 0;
    }
};






nsresult
nsStreamConverterService::FindConverter(const char *aContractID, nsTArray<nsCString> **aEdgeList) {
    nsresult rv;
    if (!aEdgeList) return NS_ERROR_NULL_POINTER;
    *aEdgeList = nullptr;

    

    uint32_t vertexCount = mAdjacencyList.Count();
    if (0 >= vertexCount) return NS_ERROR_FAILURE;

    
    BFSHashTable lBFSTable;
    mAdjacencyList.EnumerateRead(InitBFSTable, &lBFSTable);

    NS_ASSERTION(lBFSTable.Count() == vertexCount, "strmconv BFS table init problem");

    
    nsAutoCString fromC, toC;
    rv = ParseFromTo(aContractID, fromC, toC);
    if (NS_FAILED(rv)) return rv;

    BFSTableData *data = lBFSTable.Get(fromC);
    if (!data) {
        return NS_ERROR_FAILURE;
    }

    data->color = gray;
    data->distance = 0;
    CStreamConvDeallocator *dtorFunc = new CStreamConvDeallocator();

    nsDeque grayQ(dtorFunc);

    
    grayQ.Push(new nsCString(fromC));
    while (0 < grayQ.GetSize()) {
        nsCString *currentHead = (nsCString*)grayQ.PeekFront();
        nsCOMArray<nsIAtom> *data2 = mAdjacencyList.Get(*currentHead);
        if (!data2) return NS_ERROR_FAILURE;

        
        
        BFSTableData *headVertexState = lBFSTable.Get(*currentHead);
        if (!headVertexState) return NS_ERROR_FAILURE;

        int32_t edgeCount = data2->Count();

        for (int32_t i = 0; i < edgeCount; i++) {
            nsIAtom* curVertexAtom = data2->ObjectAt(i);
            nsCString *curVertex = new nsCString();
            curVertexAtom->ToUTF8String(*curVertex);

            BFSTableData *curVertexState = lBFSTable.Get(*curVertex);
            if (!curVertexState) {
                delete curVertex;
                return NS_ERROR_FAILURE;
            }

            if (white == curVertexState->color) {
                curVertexState->color = gray;
                curVertexState->distance = headVertexState->distance + 1;
                curVertexState->predecessor = new nsCString(*currentHead);
                grayQ.Push(curVertex);
            } else {
                delete curVertex; 
                                  
                                  
            }
        }
        headVertexState->color = black;
        nsCString *cur = (nsCString*)grayQ.PopFront();
        delete cur;
        cur = nullptr;
    }
    
    

    

    nsAutoCString fromStr, toMIMEType;
    rv = ParseFromTo(aContractID, fromStr, toMIMEType);
    if (NS_FAILED(rv)) return rv;

    
    nsAutoCString ContractIDPrefix(NS_ISTREAMCONVERTER_KEY);
    nsTArray<nsCString> *shortestPath = new nsTArray<nsCString>();

    data = lBFSTable.Get(toMIMEType);
    if (!data) {
        
        
        delete shortestPath;
        return NS_ERROR_FAILURE;
    }

    while (data) {
        if (fromStr.Equals(data->key)) {
            
            *aEdgeList = shortestPath;
            return NS_OK;
        }

        
        
        if (!data->predecessor) break; 
        BFSTableData *predecessorData = lBFSTable.Get(*data->predecessor);

        if (!predecessorData) break; 

        
        nsAutoCString newContractID(ContractIDPrefix);
        newContractID.AppendLiteral("?from=");

        newContractID.Append(predecessorData->key);

        newContractID.AppendLiteral("&to=");
        newContractID.Append(data->key);

        
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
        convertedData.forget(_retval);
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
        
        finalListener.forget(_retval);
    } else {
        
        rv = listener->AsyncConvertData(aFromType, aToType, aListener, aContext);
        listener.forget(_retval);
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
