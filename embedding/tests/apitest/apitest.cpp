






























#include <stdio.h>
#include <crtdbg.h>

#include "nsEmbedAPI.h"

int main(int argc, char *argv[])
{
    _CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE );
    _CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDOUT );
   

    _CrtMemState sBefore, sMiddle, sAfter;

    
    _CrtMemCheckpoint(&sBefore);

   	char *pszBinDirPath = nsnull;
	if (argc > 1)
	{
		pszBinDirPath = argv[1];
	}
	printf("apitest running...\n");
	
	nsCOMPtr<nsILocalFile> binDir;
	NS_NewLocalFile(pszBinDirPath, PR_TRUE, getter_AddRefs(binDir));
	
	nsresult rv = NS_InitEmbedding(binDir, nsnull);
	if (NS_FAILED(rv))
	{
		printf("NS_InitEmbedding FAILED (rv = 0x%08x)\n", rv);
		
	}
    else
    {
        printf("NS_InitEmbedding SUCCEEDED (rv = 0x%08x)\n", rv);
    }

	
	
    
    _CrtMemCheckpoint(&sMiddle);


	rv = NS_TermEmbedding();
	if (NS_FAILED(rv))
	{
		printf("NS_TermEmbedding FAILED (rv = 0x%08x)\n", rv);
		
	}
    else
    {
        printf("NS_TermEmbedding SUCCEEDED (rv = 0x%08x)\n", rv);
    }

	

    
    printf("FINAL LEAKAGE:\n");
    _CrtMemCheckpoint(&sAfter);
     _CrtMemState sDifference;
    if ( _CrtMemDifference(&sDifference, &sBefore, &sAfter))
    {
        _CrtMemDumpStatistics(&sDifference);
    }

    _CrtDumpMemoryLeaks( );

    printf("apitest complete\n");

	return 0;
}