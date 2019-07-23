







































#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <ctype.h>
#include <afxcoll.h>
#include <afxtempl.h>

int mainReturn = 0;
BOOL b16 = FALSE;
BOOL bSimple = FALSE;


FILE *pAltFile = stdout;

CStringArray includeDirectories;



BOOL PASCAL _AfxFullPath(LPSTR lpszPathOut, LPCSTR lpszFileIn)
        
        
        
{
        OFSTRUCT of;
        if (OpenFile(lpszFileIn, &of, OF_PARSE) != HFILE_ERROR)
        {
                
                OemToAnsi(of.szPathName, lpszPathOut);
                AnsiUpper(lpszPathOut); 
                return TRUE;
        }
        else
        {
                TRACE1("Warning: could not parse the path %Fs\n", lpszFileIn);
                lstrcpy(lpszPathOut, lpszFileIn);  
                AnsiUpper(lpszPathOut); 
                return FALSE;
        }
}

void AddIncludeDirectory( char *pString ){
    CString s = pString;
	int len = s.GetLength();
    if(len > 0 && s[len - 1] != '\\' ){
        s += "\\";
    }
    includeDirectories.Add(s);
}

BOOL FileExists( const char *pString ){
    struct _stat buf;
    int result;

    result = _stat( pString, &buf );
    return (result == 0);
}

void FixPathName(CString& str) {
	str.MakeUpper();		

	
	int index;
	while ((index = str.Find('/')) != -1) {
		str.SetAt(index, '\\');
	}
}

void FATName(CString& csLONGName)
{
    
    if(b16) {
        
        char aBuffer[2048];

        if(GetShortPathName(csLONGName, aBuffer, 2048)) {
            csLONGName = aBuffer;
        }
    }
}


class CFileRecord {
public:
    CString m_shortName;
    CString m_pathName;
    CPtrArray m_includes;  
    BOOL m_bVisited;
    BOOL m_bSystem;
    BOOL m_bSource;
    static CMapStringToPtr fileMap;      
    static CStringArray orderedFileNames;
    static CMapStringToPtr includeMap;   
    static CMapStringToPtr noDependMap;

    CFileRecord( const char *shortName, const char* pFullName, BOOL bSystem, BOOL bSource):
                m_shortName( shortName ),
                m_pathName(),
                m_includes(),
                m_bVisited(FALSE),
                m_bSource( bSource ),
                m_bSystem(bSystem){

		m_pathName = pFullName;
		FixPathName(m_pathName);				
		ASSERT(FindFileRecord(m_pathName) == NULL);	
        fileMap[m_pathName] = this;			
		if (bSource) {
			orderedFileNames.Add(m_pathName);	
		}
    }

    
    
    
    void ProcessFile(){
        FILE *f;
		CString fullName;
        BOOL bSystem;
		DWORD lineCntr = 0;
		char *a = new char[2048];
        memset(a, 0, 2048);
		char srcPath[_MAX_PATH];

		
		if (!_AfxFullPath(srcPath, m_pathName)) {
			strcpy(srcPath, m_pathName);
		}

		
		LPSTR pTemp = strrchr(srcPath, '\\');
		if (pTemp) {
			*(pTemp + 1) = 0;
		}

        f = fopen(m_pathName, "r");
        if(f != NULL && f != (FILE *)-1)  {
			setvbuf(f, NULL, _IOFBF, 32768);		
            while(fgets(a, 2047, f)) {
				
				
				
				if (lineCntr < 10) {
					static char* pDependStr = "//{{NO_DEPENDENCIES}}";
					if (strncmp(a, pDependStr, strlen(pDependStr)) == 0) {
						noDependMap[m_pathName] = 0;	
						break;							
					}
				}
				++lineCntr;
				
				
				
                
				
				pTemp = a;
				pTemp += strspn(pTemp, " \t");			
				if (*pTemp == '#') {
					++pTemp;							
					pTemp += strspn(pTemp, " \t");		
					if( !strncmp(pTemp, "include", 7) ){
						pTemp += 7;						
						pTemp += strspn(pTemp, " \t");	
						bSystem = (*pTemp == '<');		
                        
                        
                        
						
                        if (*pTemp == '"') {
							LPSTR pStart = pTemp + 1;	
							pTemp = pStart + strcspn(pStart, ">\" ");	
							*pTemp = 0;					

							
							
							fullName = srcPath;
							fullName += pStart;
							CFileRecord *pAddMe = AddFile( pStart, fullName, bSystem );
							if (pAddMe) {
								m_includes.Add(pAddMe);
							}
						}
					}
				}
            }
            fclose(f);
        }
        delete [] a;
    }

    void PrintIncludes(){
        int i = 0;
        while( i < m_includes.GetSize() ){
            CFileRecord *pRec = (CFileRecord*) m_includes[i];

            
            
            
			
			void*	lookupJunk;
            if( !pRec->m_bVisited && pRec->m_pathName.GetLength() != 0 && !noDependMap.Lookup(pRec->m_pathName, lookupJunk)) {

				
				ASSERT(FileExists(pRec->m_pathName));

                CString csOutput;
                csOutput = pRec->m_pathName;
                FATName(csOutput);

				fprintf(pAltFile, "\\\n    %s ", (const char *) csOutput );

				
                pRec->m_bVisited = TRUE;

                pRec->PrintIncludes();
            }
            i++;
        }
    }

    void PrintDepend(){
        CFileRecord *pRec;
        BOOL bFound;
        POSITION next;
        CString name;

		
		
        next = fileMap.GetStartPosition();
        while( next ){
            fileMap.GetNextAssoc( next, name, *(void**)&pRec );
            pRec->m_bVisited = FALSE;
        }

        char fname[_MAX_FNAME];

		if (pRec->m_pathName.GetLength() != 0) {
            if( bSimple ){
    			fprintf(pAltFile, "\n\n\n%s:\t", m_pathName );
            }
            else {
                CString csOutput;
                csOutput = m_pathName;
                FATName(csOutput);

    			_splitpath( csOutput, NULL, NULL, fname, NULL );

    			fprintf(pAltFile, "\n\n\n$(OUTDIR)\\%s.obj: %s ", fname, (const char*) csOutput );
            }
	        m_bVisited = TRUE;		
	        PrintIncludes();
		}
    }


    static CString NormalizeFileName( const char* pName ){
        return CString(pName);
    }

    static CFileRecord* FindFileRecord( const char *pName ){
		CFileRecord* pRec = NULL;
		CString name(pName);
		FixPathName(name);
		fileMap.Lookup(name, (void*&)pRec);
		return(pRec);
    }
public:
    static CFileRecord* AddFile( const char* pShortName, const char* pFullName, BOOL bSystem = FALSE, 
                BOOL bSource = FALSE ){

		char fullName[_MAX_PATH];
		BOOL bFound = FALSE;
		CString foundName;
		CString fixedShortName;
        CString s;

        
        fixedShortName = pShortName;
        FixPathName(fixedShortName);
        pShortName = fixedShortName;

        
        
        if( bSource && (strcmp(GetExt(pShortName),".obj") == 0) ){
            char path_buffer[_MAX_PATH];
            char fname[_MAX_FNAME] = "";
            CString s;

            _splitpath( pShortName, NULL, NULL, fname, NULL );
            if( FileExists( s = CString(fname) + ".cpp") ){
                pShortName = s;
                pFullName = s;
            }
            else if( FileExists( s = CString(fname) + ".c" ) ){
                pShortName = s;
                pFullName = s;
            }
            else {
                return 0;
            }
        }

		
		if (!pFullName) {
			_AfxFullPath(fullName, pShortName);
			pFullName = fullName;
		}
		
		
		CFileRecord *pRec = FindFileRecord(pFullName);

        
        
        
        if (!pRec && !bSource)
            includeMap.Lookup(fixedShortName, (void*&)pRec);

        if (!pRec) {
            

            
            if (FileExists(pFullName)) {
                foundName = pFullName;
                bFound = TRUE;
            }
            else {
                
                int i = 0;
                while( i < includeDirectories.GetSize() ){
                    if( FileExists( includeDirectories[i] + pShortName ) ){
                        foundName = includeDirectories[i] + pShortName;
                        bFound = TRUE;
                        break;
                    }
                    i++;
                }
            }
        }
        else {
            
            bFound = TRUE;
        }

		
		if (bSource && !pRec && !bFound) {
			fprintf(stderr, "Source file: %s doesn't exist\n", pFullName);
			mainReturn = -1;		
		}

#ifdef _DEBUG
		if (!pRec && !bFound && !bSystem) {
			fprintf(stderr, "Header not found: %s (%s)\n", pShortName, pFullName);
		}
#endif

		
        
        if (bFound && (pRec == NULL)) {
            pRec = new CFileRecord( pShortName, foundName, bSystem, bSource);

			
			
			
			if (!bSource) {
				includeMap[pShortName] = pRec;
			}
        }
        return pRec;
    }


    static void PrintDependancies(){
        CFileRecord *pRec;
        BOOL bFound;
        POSITION next;
        CString name;

		
		for (int pos = 0; pos < orderedFileNames.GetSize(); pos++) {
			pRec = FindFileRecord(orderedFileNames[pos]);
            if(pRec && pRec->m_bSource ){
                pRec->PrintDepend();
			}
		}
    }


    void PrintDepend2(){
        CFileRecord *pRec;
        int i;

        if( m_includes.GetSize() != 0 ){
			fprintf(pAltFile, "\n\n\n%s: \\\n",m_pathName );
            i = 0;
            while( i < m_includes.GetSize() ){
                pRec = (CFileRecord*) m_includes[i];
    			fprintf(pAltFile, "\t\t\t%s\t\\\n",pRec->m_pathName );
                i++;
            }
        }
    }

    static void PrintDependancies2(){
        CFileRecord *pRec;
        BOOL bFound;
        POSITION next;
        CString name;

        next = fileMap.GetStartPosition();
        while( next ){
            fileMap.GetNextAssoc( next, name, *(void**)&pRec );
            pRec->PrintDepend2();
        }
    }


    static void PrintTargets(const char *pMacroName, const char *pDelimiter){
        CFileRecord *pRec;
        BOOL bFound;
        POSITION next;
        CString name;

        BOOL bNeedDelimiter = FALSE;
		fprintf(pAltFile, "%s = ", pMacroName);        

		
		for (int pos = 0; pos < orderedFileNames.GetSize(); pos++) {
			pRec = FindFileRecord(orderedFileNames[pos]);
			ASSERT(pRec);

            if( pRec && pRec->m_bSource && pRec->m_pathName.GetLength() != 0){
                char fname[_MAX_FNAME];

                CString csOutput;
                csOutput = pRec->m_pathName;
                FATName(csOutput);

                _splitpath( csOutput, NULL, NULL, fname, NULL );

                if(bNeedDelimiter)  {
                    fprintf(pAltFile, "%s\n", pDelimiter);
                    bNeedDelimiter = FALSE;
                }

				fprintf(pAltFile, "     $(OUTDIR)\\%s.obj   ", fname );
                bNeedDelimiter = TRUE;
            }
        }
		fprintf(pAltFile, "\n\n\n");        
    }

    static CString DirDefine( const char *pPath ){
        char path_buffer[_MAX_PATH];
        char dir[_MAX_DIR] = "";
        char dir2[_MAX_DIR] = "";
        char fname[_MAX_FNAME] = "";
        char ext[_MAX_EXT] = "";
        CString s;

        _splitpath( pPath, 0, dir, 0, ext );

        BOOL bDone = FALSE;

        while( dir && !bDone){
            
            dir[ strlen(dir)-1] = 0;
            _splitpath( dir, 0, dir2, fname, 0 );
            if( strcmp( fname, "SRC" ) == 0 ){
                strcpy( dir, dir2 );
            }
            else {
                bDone = TRUE;
            }
        }
        s = CString(fname) + "_" + (ext+1);
        return s;
    }


    static void PrintSources(){
        int i;
        CString dirName, newDirName;

        for( i=0; i< orderedFileNames.GetSize(); i++ ){
            newDirName= DirDefine( orderedFileNames[i] );
            if( newDirName != dirName ){
                fprintf( pAltFile, "\n\n\nFILES_%s= $(FILES_%s) \\", 
                        (const char*)newDirName, (const char*)newDirName );
                dirName = newDirName;
            }
            fprintf( pAltFile, "\n\t%s^", (const char*)orderedFileNames[i] );
        }
    }

    static CString SourceDirName( const char *pPath, BOOL bFileName){
        char path_buffer[_MAX_PATH];
        char drive[_MAX_DRIVE] = "";
        char dir[_MAX_DIR] = "";
        char fname[_MAX_FNAME] = "";
        char ext[_MAX_EXT] = "";
        CString s;

        _splitpath( pPath, drive, dir, fname, ext );

        s = CString(drive) + dir;
        if( bFileName ){
            s += CString("FNAME") + ext;
        }
        else {
            
            s = s.Left( s.GetLength() - 1 );
        }
        return s;
    }


    static CString GetExt( const char *pPath){
        char ext[_MAX_EXT] = "";

        _splitpath( pPath, 0,0,0, ext );

        CString s = CString(ext);
        s.MakeLower();
        return s;
    }

    static void PrintBuildRules(){
        int i;
        CString dirName;
        
        CMapStringToPtr dirList;

        for( i=0; i< orderedFileNames.GetSize(); i++ ){
            dirList[ SourceDirName(orderedFileNames[i], TRUE) ]= 0;
        }

        POSITION next;
        CString name;
        void *pVal;

        next = dirList.GetStartPosition();
        while( next ){
            dirList.GetNextAssoc( next, name, pVal);
            CString dirDefine = DirDefine( name );
            CString ext = GetExt( name );
            name = SourceDirName( name, FALSE );
            CString response = dirDefine.Left(8);

            fprintf( pAltFile, 
                "\n\n\n{%s}%s{$(OUTDIR)}.obj:\n"
                "\t@rem <<$(OUTDIR)\\%s.cl\n"
                "\t$(CFILEFLAGS)\n"
                "\t$(CFLAGS_%s)\n"
                "<<KEEP\n"
                "\t$(CPP) @$(OUTDIR)\\%s.cl %%s\n",
                (const char*)name,
                (const char*)ext,
                (const char*)response,
                (const char*)dirDefine,
                (const char*)response
            );

            fprintf( pAltFile, 
                "\n\n\nBATCH_%s:\n"
                "\t@rem <<$(OUTDIR)\\%s.cl\n"
                "\t$(CFILEFLAGS)\n"
                "\t$(CFLAGS_%s)\n"
                "\t$(FILES_%s)\n"
                "<<KEEP\n"
                "\t$(TIMESTART)\n"
                "\t$(CPP) @$(OUTDIR)\\%s.cl\n"
                "\t$(TIMESTOP)\n",
                (const char*)dirDefine,
                (const char*)response,
                (const char*)dirDefine,
                (const char*)dirDefine,
                (const char*)response
            );
        }

        
        
        
        
        fprintf( pAltFile, 
            "\n\n\nBATCH_BUILD_OBJECTS:\t\t\\\n");

        next = dirList.GetStartPosition();
        while( next ){
            dirList.GetNextAssoc( next, name, pVal);
            CString dirDefine = DirDefine( name );

            fprintf( pAltFile, 
                "\tBATCH_%s\t\t\\\n", dirDefine );
        }

        fprintf( pAltFile, 
            "\n\n");
    }
        

    static void ProcessFiles(){
        CFileRecord *pRec;
        BOOL bFound;
        POSITION next;
        CString name;

		
		
		

        next = fileMap.GetStartPosition();
        while( next ){
            fileMap.GetNextAssoc( next, name, *(void**)&pRec );
            if( pRec->m_bVisited == FALSE && pRec->m_bSystem == FALSE ){
				
				
                pRec->m_bVisited = TRUE;
                pRec->ProcessFile();
                
				
				
                next = fileMap.GetStartPosition();       

            }
        }
    }


};

CMapStringToPtr CFileRecord::fileMap;           
CStringArray CFileRecord::orderedFileNames;
CMapStringToPtr CFileRecord::includeMap;        
CMapStringToPtr CFileRecord::noDependMap;       

int main( int argc, char** argv ){
    int i = 1;
    char *pStr;
    static int iRecursion = 0;	
	static CString outputFileName;
    
    
    iRecursion++;

    while( i < argc ){
        if( argv[i][0] == '-' || argv[i][0] == '/' ){
            switch( argv[i][1] ){

            case 'i':
            case 'I':
                if( argv[i][2] != 0 ){
                    pStr = &(argv[i][2]);
                }
                else {
                    i++;
                    pStr = argv[i];
                }
                if( pStr == 0 || *pStr == '-' || *pStr == '/' ){
                    goto usage;
                }
                else {
                    AddIncludeDirectory( pStr );
                }
                break;

            case 'f':
            case 'F':
                if( argv[i][2] != 0 ){
                    pStr = &(argv[i][2]);
                }
                else {
                    i++;
                    pStr = argv[i];
                }
                if( pStr == 0 || *pStr == '-' || *pStr == '/'){
                    goto usage;
                }
                else {
                    CStdioFile f;
                    CString s;
                    if( f.Open( pStr, CFile::modeRead ) ){
                        while(f.ReadString(s)){
                            s.TrimLeft();
                            s.TrimRight();
                            if( s.GetLength() ){
                                CFileRecord::AddFile( s, NULL, FALSE, TRUE );
                            }
                        } 
                        f.Close();
                    }
                    else {
                        fprintf(stderr,"makedep: file not found: %s", pStr );
                        exit(-1);
                    }
                }
                break;

            case 'o':
            case 'O':
                if( argv[i][2] != 0 ){
                    pStr = &(argv[i][2]);
                }
                else {
                    i++;
                    pStr = argv[i];
                }
                if( pStr == 0 || *pStr == '-' || *pStr == '/'){
                    goto usage;
                }
                else {
                    CStdioFile f;
                    CString s;
					outputFileName = pStr;
					if(!(pAltFile = fopen(pStr, "w+")))	{
                        fprintf(stderr, "makedep: file not found: %s", pStr );
                        exit(-1);
                    }
                }
                break;

            case '1':
                if( argv[i][2] == '6')  {
                    b16 = TRUE;
                }
                break;

            case 's':
            case 'S':
                bSimple = TRUE;
                break;



            case 'h':
            case 'H':
            case '?':
            usage:
                fprintf(stderr, "usage: makedep -I <dirname> -F <filelist> <filename>\n"
                       "  -I <dirname>    Directory name, can be repeated\n"
                       "  -F <filelist>   List of files to scan, one per line\n"
                       "  -O <outputFile> File to write output, default stdout\n");
                exit(-1);
            }
        }
        else if( argv[i][0] == '@' ){
        	
	        CStdioFile f;
    	    CString s;
    	    int iNewArgc = 0;
    	    char **apNewArgv = new char*[5000];
			memset(apNewArgv, 0, sizeof(apNewArgv));

			
			apNewArgv[0] = argv[0];
			iNewArgc++;

			const char *pTraverse;
			const char *pBeginArg;
	        if( f.Open( &argv[i][1], CFile::modeRead ) ){
    	        while( iNewArgc < 5000 && f.ReadString(s) )	{
					
					pTraverse = (const char *)s;
					while(iNewArgc < 5000 && *pTraverse)	{
						if(isspace(*pTraverse))	{
								pTraverse++;
								continue;
						}

						
						pBeginArg = pTraverse;
						do	{
							pTraverse++;
						}
						while(*pTraverse && !isspace(*pTraverse));
						apNewArgv[iNewArgc] = new char[pTraverse - pBeginArg + 1];
						memset(apNewArgv[iNewArgc], 0, pTraverse - pBeginArg + 1);
						strncpy(apNewArgv[iNewArgc], pBeginArg, pTraverse - pBeginArg);
						iNewArgc++;
					}
	            } 
    	        f.Close();
        	}
        	
        	
        	if(iNewArgc > 1)	{
        		main(iNewArgc, apNewArgv);
        	}
        	
        	
        	while(iNewArgc > 1)	{
        		iNewArgc--;
        		delete [] apNewArgv[iNewArgc];
        	}
        	delete [] apNewArgv;
        }
        else {
            CFileRecord::AddFile( argv[i], NULL, FALSE, TRUE );
        }
        i++;
    }
    
    
    if(iRecursion == 1)	{

		
		if (mainReturn == 0) {
			CFileRecord::ProcessFiles();
            if( !bSimple ){
        		CFileRecord::PrintTargets("OBJ_FILES", "\\");
                if(b16) {
    			    CFileRecord::PrintTargets("LINK_OBJS", "+\\");
                }
                else    {
    			    CFileRecord::PrintTargets("LINK_OBJS", "^");
                }
                CFileRecord::PrintSources();
                CFileRecord::PrintBuildRules();
            }
    		CFileRecord::PrintDependancies();
		}
	    
		if(pAltFile != stdout)	{
			fclose(pAltFile);
			if (mainReturn != 0) {
				remove(outputFileName);	
			}
		}
	}
	iRecursion--;

    if (iRecursion == 0 )
    {
        
        CFileRecord *pFRec;
        CString     name;
        POSITION    next;

        next = CFileRecord::fileMap.GetStartPosition();
        while( next ){
            CFileRecord::fileMap.GetNextAssoc( next, name, *(void**)&pFRec );
            delete pFRec;
        }
    }

    return mainReturn;
}
