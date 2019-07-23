




































#include "nsIServiceManager.h"
#include "nsCOMPtr.h"
#include "nsIChromeRegistry.h"

#include "rdf.h"
#include "nsIRDFResource.h"
#include "nsIRDFService.h"
#include "nsIRDFContainer.h"

#include "nsIFile.h"
#include "nsAppDirectoryServiceDefs.h"

#include "nsNetUtil.h"

#include "nsString.h"

#include "plgetopt.h"

#include <stdlib.h>             

static const char uri_prefix[] = "http://www.mozilla.org/rdf/chrome#";
static const char urn_prefix[] = "urn:mozilla:";







#define NS_CHROMEREGISTRY_CID \
{ 0xd8c7d8a2, 0xe84c, 0x11d2, { 0xbf, 0x87, 0x0, 0x10, 0x5a, 0x1b, 0x6, 0x27 } }

static NS_DEFINE_CID(kChromeRegistryCID, NS_CHROMEREGISTRY_CID);

nsresult
WritePropertiesTo(nsIRDFDataSource*, const char* aProviderType, FILE* out);

nsresult
WriteAttributes(nsIRDFDataSource*, const char* aProviderType, const char* aProvider,
                nsIRDFResource* aResource,
                FILE* out);

void
TranslateResourceValue(const char* aProviderType,
                       const char* aProvider,
                       const char* aArc,
                       const char* aResourceValue,
                       nsACString& aResult);

nsresult WriteProperties(const char* properties_file);

void print_help();

int main(int argc, char **argv)
{
    const char* properties_filename = nsnull;

    PLOptState* optstate = PL_CreateOptState(argc, argv, "p:");
    
    while (PL_GetNextOpt(optstate) == PL_OPT_OK) {
        switch (optstate->option) {
        case 'p':               
            properties_filename = optstate->value;
            break;
        case '?':
            print_help();
            exit(1);
            break;
        }
    }
    PL_DestroyOptState(optstate);

    NS_InitXPCOM2(nsnull, nsnull, nsnull);

    nsCOMPtr <nsIChromeRegistry> chromeReg = 
        do_GetService(kChromeRegistryCID);
    if (!chromeReg) {
        NS_WARNING("chrome check couldn't get the chrome registry");
        return NS_ERROR_FAILURE;
    }
    chromeReg->CheckForNewChrome();

    
    if (properties_filename)
        WriteProperties(properties_filename);

    
    chromeReg = 0;
    NS_ShutdownXPCOM(nsnull);
    return 0;
}


nsresult
WritePropertiesTo(nsIRDFDataSource* aDataSource,
                  const char* aProviderType, FILE* out)
{

    
    nsCOMPtr<nsIRDFService> rdfService =
        do_GetService("@mozilla.org/rdf/rdf-service;1");
    
    nsCOMPtr<nsIRDFResource> providerRoot;
    nsCAutoString urn(NS_LITERAL_CSTRING(urn_prefix) +
                      nsDependentCString(aProviderType) + NS_LITERAL_CSTRING(":root"));

    rdfService->GetResource(urn,
                            getter_AddRefs(providerRoot));

    nsCOMPtr<nsIRDFContainer> providerContainer =
        do_CreateInstance("@mozilla.org/rdf/container;1");

    providerContainer->Init(aDataSource, providerRoot);

    nsCOMPtr<nsISimpleEnumerator> providers;
    providerContainer->GetElements(getter_AddRefs(providers));

    PRBool hasMore;
    providers->HasMoreElements(&hasMore);
    for (; hasMore; providers->HasMoreElements(&hasMore)) {
        nsCOMPtr<nsISupports> supports;
        providers->GetNext(getter_AddRefs(supports));

        nsCOMPtr<nsIRDFResource> kid = do_QueryInterface(supports);

        const char* providerUrn;
        kid->GetValueConst(&providerUrn);
        
        
        
        providerUrn += (sizeof(urn_prefix)-1) + strlen(aProviderType) + 1;

        WriteAttributes(aDataSource, aProviderType, providerUrn, kid, out);
    }

    return NS_OK;
}


nsresult
WriteAttributes(nsIRDFDataSource* aDataSource,
                const char* aProviderType,
                const char* aProvider,
                nsIRDFResource* aResource, FILE* out)
{
    nsresult rv;
    
    nsCOMPtr<nsISimpleEnumerator> arcs;
    rv = aDataSource->ArcLabelsOut(aResource, getter_AddRefs(arcs));
    if (NS_FAILED(rv))
        return rv;

    PRBool hasMore;
    rv = arcs->HasMoreElements(&hasMore);
    if (NS_FAILED(rv))
        return rv;


    for (; hasMore; arcs->HasMoreElements(&hasMore)) {
        nsCOMPtr<nsISupports> supports;
        arcs->GetNext(getter_AddRefs(supports));

        nsCOMPtr<nsIRDFResource> arc = do_QueryInterface(supports);

        const char* arcValue;
        arc->GetValueConst(&arcValue);
        arcValue += (sizeof(uri_prefix)-1); 
         
        
        nsCOMPtr<nsIRDFNode> valueNode;
        aDataSource->GetTarget(aResource, arc, PR_TRUE, 
                               getter_AddRefs(valueNode));
         
        nsCOMPtr<nsIRDFLiteral> literal = do_QueryInterface(valueNode);
        if (literal) {
            const PRUnichar* literalValue;
            literal->GetValueConst(&literalValue);
            fprintf(out, "%s.%s.%s=%s\n", aProviderType, aProvider, arcValue, NS_ConvertUTF16toUTF8(literalValue).get());
        }

        nsCOMPtr<nsIRDFResource> resource = do_QueryInterface(valueNode);
        if (resource) {
            const char* resourceValue;
            resource->GetValueConst(&resourceValue);
            
            nsCAutoString translatedValue;
            TranslateResourceValue(aProviderType, aProvider,
                                   arcValue, resourceValue, translatedValue);
            fprintf(out, "%s.%s.%s=%s\n", aProviderType, aProvider, arcValue, translatedValue.get());
        }
    }

    return NS_OK;
}
    
nsresult WriteProperties(const char* properties_file)
{
    FILE* props;
    printf("writing to %s\n", properties_file);
    if (!(props = fopen(properties_file, "w"))) {
        printf("Could not write to %s\n", properties_file);
        return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsIFile> chromeFile;
    nsresult rv = NS_GetSpecialDirectory(NS_APP_CHROME_DIR, getter_AddRefs(chromeFile));
    if(NS_FAILED(rv)) {
        fclose(props);
        return rv;
    }

    chromeFile->AppendNative(NS_LITERAL_CSTRING("chrome.rdf"));
    
    nsCAutoString pathURL;
    NS_GetURLSpecFromFile(chromeFile, pathURL);

    nsCOMPtr<nsIRDFService> rdf = 
        do_GetService("@mozilla.org/rdf/rdf-service;1");

    nsCOMPtr<nsIRDFDataSource> chromeDS;
    rdf->GetDataSource(pathURL.get(), getter_AddRefs(chromeDS));

    WritePropertiesTo(chromeDS, "package", props);
    WritePropertiesTo(chromeDS, "skin", props);
    WritePropertiesTo(chromeDS, "locale", props);

    fclose(props);
    return NS_OK;
}

void print_help() {

    printf("Usage: regchrome [-p file.properties]\n"
           "Registers chrome by scanning installed-chrome.txt\n"
           "\n"
           "Options: \n"
           "  -p file.properties    Output a flat-file version of the registry to\n"
           "                        the specified file\n");

}

void
TranslateResourceValue(const char* aProviderType,
                       const char* aProvider,
                       const char* aArc,
                       const char* aResourceValue,
                       nsACString& aResult)
{
    PRUint32 chopStart=0, chopEnd=0; 

    static const char localeUrn[] = "urn:mozilla:locale:";
    if ((strcmp(aArc, "selectedLocale") == 0) &&
        (strncmp(aResourceValue, localeUrn, sizeof(localeUrn)-1) == 0)) {
        
        chopStart = sizeof(localeUrn) - 1;
        chopEnd = strlen(aProvider) + 1;
    }

    static const char skinUrn[] = "urn:mozilla:skin:";
    if ((strcmp(aArc, "selectedSkin") == 0) &&
        (strncmp(aResourceValue, skinUrn, sizeof(skinUrn)-1) == 0)) {

        chopStart = sizeof(skinUrn) - 1;
        chopEnd = strlen(aProvider) + 1;
    }

    
    aResult = (aResourceValue + chopStart);
    aResult.Truncate(aResult.Length() - chopEnd);
        
}
