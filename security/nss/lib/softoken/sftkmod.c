


















































#include "sftkdb.h"
#include "sftkpars.h"
#include "prprf.h" 
#include "prsystem.h"
#include "lgglue.h"
#include "secmodt.h"
#if defined (_WIN32)
#include <io.h>
#endif




















static char *
sftkdb_quote(const char *string, char quote)
{
    char *newString = 0;
    int escapes = 0, size = 0;
    const char *src;
    char *dest;

    size=2;
    for (src=string; *src ; src++) {
	if ((*src == quote) || (*src == '\\')) escapes++;
	size++;
    }

    dest = newString = PORT_ZAlloc(escapes+size+1); 
    if (newString == NULL) {
	return NULL;
    }

    *dest++=quote;
    for (src=string; *src; src++,dest++) {
	if ((*src == '\\') || (*src == quote)) {
	    *dest++ = '\\';
	}
	*dest = *src;
    }
    *dest=quote;

    return newString;
}







static char *
sftkdb_DupnCat(char *baseString, const char *str, int str_len)
{
    int len = (baseString ? PORT_Strlen(baseString) : 0) + 1;
    char *newString;

    len += str_len;
    newString = (char *) PORT_Realloc(baseString,len);
    if (newString == NULL) {
	PORT_Free(baseString);
	return NULL;
    }
    if (baseString == NULL) *newString = 0;
    return PORT_Strncat(newString,str, str_len);
}



static char *
sftkdb_DupCat(char *baseString, const char *str)
{
    return sftkdb_DupnCat(baseString, str, PORT_Strlen(str));
}



static SECStatus
sftkdb_releaseSpecList(char **moduleSpecList)
{
    if (moduleSpecList) {
	char **index;
	for(index = moduleSpecList; *index; index++) {
	    PORT_Free(*index);
	}
	PORT_Free(moduleSpecList);
    }
    return SECSuccess;
}

#define SECMOD_STEP 10
static SECStatus
sftkdb_growList(char ***pModuleList, int *useCount, int last)
{
    char **newModuleList;

    *useCount += SECMOD_STEP;
    newModuleList = (char **)PORT_Realloc(*pModuleList,
					  *useCount*sizeof(char *));
    if (newModuleList == NULL) {
	return SECFailure;
    }
    PORT_Memset(&newModuleList[last],0, sizeof(char *)*SECMOD_STEP);
    *pModuleList = newModuleList;
    return SECSuccess;
}

static 
char *sftk_getOldSecmodName(const char *dbname,const char *filename)
{
    char *file = NULL;
    char *dirPath = PORT_Strdup(dbname);
    char *sep;

    sep = PORT_Strrchr(dirPath,*PATH_SEPARATOR);
#ifdef WINDOWS
    if (!sep) {
	sep = PORT_Strrchr(dirPath,'/');
    }
#endif
    if (sep) {
	*(sep)=0;
    }
    file= PR_smprintf("%s"PATH_SEPARATOR"%s", dirPath, filename);
    PORT_Free(dirPath);
    return file;
}

#ifdef XP_UNIX
#include <unistd.h>
#endif
#include <fcntl.h>

#ifndef WINCE

FILE *
lfopen(const char *name, const char *mode, int flags)
{
    int fd;
    FILE *file;

    fd = open(name, flags, 0600);
    if (fd < 0) {
	return NULL;
    }
    file = fdopen(fd, mode);
    if (!file) {
	close(fd);
    }
    
    return file;
}
#endif

#define MAX_LINE_LENGTH 2048
#define SFTK_DEFAULT_INTERNAL_INIT1 "library= name=\"NSS Internal PKCS #11 Module\" parameters="
#define SFTK_DEFAULT_INTERNAL_INIT2 " NSS=\"Flags=internal,critical trustOrder=75 cipherOrder=100 slotParams=(1={"
#define SFTK_DEFAULT_INTERNAL_INIT3 " askpw=any timeout=30})\""




char **
sftkdb_ReadSecmodDB(SDBType dbType, const char *appName, 
		    const char *filename, const char *dbname, 
		    char *params, PRBool rw)
{
    FILE *fd = NULL;
    char **moduleList = NULL;
    int moduleCount = 1;
    int useCount = SECMOD_STEP;
    char line[MAX_LINE_LENGTH];
    PRBool internal = PR_FALSE;
    PRBool skipParams = PR_FALSE;
    char *moduleString = NULL;
    char *paramsValue=NULL;
    PRBool failed = PR_TRUE;

    if ((dbType == SDB_LEGACY) || (dbType == SDB_MULTIACCESS)) {
	return sftkdbCall_ReadSecmodDB(appName, filename, dbname, params, rw);
    }

    moduleList = (char **) PORT_ZAlloc(useCount*sizeof(char **));
    if (moduleList == NULL) return NULL;

    
    fd = fopen(dbname, "r");
    if (fd == NULL) goto done;

    



    
    moduleString = NULL;  
    internal = PR_FALSE;	     
    skipParams = PR_FALSE;	   
    paramsValue = NULL;		   
    while (fgets(line, sizeof(line), fd) != NULL) { 
	int len = PORT_Strlen(line);

	
	if (len && line[len-1] == '\n') {
	    len--;
	    line[len] = 0;
	}
	if (*line == '#') {
	    continue;
	}
	if (*line != 0) {
	    





	    char *value = PORT_Strchr(line,'=');

	    
	    if (value == NULL || value[1] == 0) {
		if (moduleString) {
		    moduleString = sftkdb_DupnCat(moduleString," ", 1);
		    if (moduleString == NULL) goto loser;
		}
	        moduleString = sftkdb_DupCat(moduleString, line);
		if (moduleString == NULL) goto loser;
	    
	    } else if (value[1] == '"') {
		if (moduleString) {
		    moduleString = sftkdb_DupnCat(moduleString," ", 1);
		    if (moduleString == NULL) goto loser;
		}
	        moduleString = sftkdb_DupCat(moduleString, line);
		if (moduleString == NULL) goto loser;
		


	        if (PORT_Strncasecmp(line, "parameters", 10) == 0) {
			skipParams = PR_TRUE;
		}
	    

















	    } else if (PORT_Strncasecmp(line, "parameters", 10) == 0) {
		
		if (paramsValue) {
			continue;
		}
		paramsValue = sftkdb_quote(&value[1], '"');
		if (paramsValue == NULL) goto loser;
		continue;
	    } else {
	    
	        char *newLine;
		if (moduleString) {
		    moduleString = sftkdb_DupnCat(moduleString," ", 1);
		    if (moduleString == NULL) goto loser;
		}
		moduleString = sftkdb_DupnCat(moduleString,line,value-line+1);
		if (moduleString == NULL)  goto loser;
	        newLine = sftkdb_quote(&value[1],'"');
		if (newLine == NULL) goto loser;
		moduleString = sftkdb_DupCat(moduleString,newLine);
	        PORT_Free(newLine);
		if (moduleString == NULL) goto loser;
	    }

	    
	    if (PORT_Strncasecmp(line, "NSS=", 4) == 0) {
		

		if (PORT_Strstr(line,"internal")) {
		    internal = PR_TRUE;
		    
		    if (paramsValue) {
			PORT_Free(paramsValue);
		    }
		    paramsValue = sftkdb_quote(params, '"');
		}
	    }
	    continue;
	}
	if ((moduleString == NULL) || (*moduleString == 0)) {
	    continue;
	}

	



	if (paramsValue) {
	    
	    if (!skipParams) {
		moduleString = sftkdb_DupnCat(moduleString," parameters=", 12);
		if (moduleString == NULL) goto loser;
		moduleString = sftkdb_DupCat(moduleString, paramsValue);
		if (moduleString == NULL) goto loser;
	    }
	    PORT_Free(paramsValue);
	    paramsValue = NULL;
	}

	if ((moduleCount+1) >= useCount) {
	    SECStatus rv;
	    rv = sftkdb_growList(&moduleList, &useCount,  moduleCount+1);
	    if (rv != SECSuccess) {
		goto loser;
	    }
	}

	if (internal) {
	    moduleList[0] = moduleString;
	} else {
	    moduleList[moduleCount] = moduleString;
	    moduleCount++;
	}
	moduleString = NULL;
	internal = PR_FALSE;
	skipParams = PR_FALSE;
    } 

    if (moduleString) {
	PORT_Free(moduleString);
	moduleString = NULL;
    }
done:
    
    if (fd == NULL) {
	char *olddbname = sftk_getOldSecmodName(dbname,filename);
	PRStatus status;
	char **oldModuleList;
	int i;

	
	if (!olddbname) {
	    goto bail;
	}

	
	status = PR_Access(olddbname, PR_ACCESS_EXISTS);
	if (status != PR_SUCCESS) {
	    goto bail;
	}

	oldModuleList = sftkdbCall_ReadSecmodDB(appName, filename, 
					olddbname, params, rw);
	
	if (!oldModuleList) {
	    goto bail;
	}

	
	for (i=0; oldModuleList[i]; i++) { }

	
	if (i >= useCount) {
	    SECStatus rv;
	    rv = sftkdb_growList(&moduleList,&useCount,moduleCount+1);
	    if (rv != SECSuccess) {
		goto loser;
	    }
	}
	
	
	for (i=0; oldModuleList[i]; i++) {
	    if (rw) {
		sftkdb_AddSecmodDB(dbType,appName,filename,dbname,
				oldModuleList[i],rw);
	    }
	    if (moduleList[i]) {
		PORT_Free(moduleList[i]);
	    }
	    moduleList[i] = PORT_Strdup(oldModuleList[i]);
	}

	
	sftkdbCall_ReleaseSecmodDBData(appName, filename, olddbname, 
				  oldModuleList, rw);
bail:
	if (olddbname) {
	    PR_smprintf_free(olddbname);
	}
    }
	
    if (!moduleList[0]) {
	char * newParams;
	moduleString = PORT_Strdup(SFTK_DEFAULT_INTERNAL_INIT1);
	newParams = sftkdb_quote(params,'"');
	if (newParams == NULL) goto loser;
	moduleString = sftkdb_DupCat(moduleString, newParams);
	PORT_Free(newParams);
	if (moduleString == NULL) goto loser;
	moduleString = sftkdb_DupCat(moduleString, SFTK_DEFAULT_INTERNAL_INIT2);
	if (moduleString == NULL) goto loser;
	moduleString = sftkdb_DupCat(moduleString, SECMOD_SLOT_FLAGS);
	if (moduleString == NULL) goto loser;
	moduleString = sftkdb_DupCat(moduleString, SFTK_DEFAULT_INTERNAL_INIT3);
	if (moduleString == NULL) goto loser;
	moduleList[0] = moduleString;
	moduleString = NULL;
    }
    failed = PR_FALSE;

loser:
    


    
    if (moduleString) {
	PORT_Free(moduleString);
	moduleString = NULL;
    }
    if (paramsValue) {
	PORT_Free(paramsValue);
	paramsValue = NULL;
    }
    if (failed || (moduleList[0] == NULL)) {
	
	sftkdb_releaseSpecList(moduleList);
	moduleList = NULL;
	failed = PR_TRUE;
    }
    if (fd != NULL) {
	fclose(fd);
    } else if (!failed && rw) {
	
	sftkdb_AddSecmodDB(dbType,appName,filename,dbname,moduleList[0],rw);
    }
    return moduleList;
}

SECStatus
sftkdb_ReleaseSecmodDBData(SDBType dbType, const char *appName, 
			const char *filename, const char *dbname, 
			char **moduleSpecList, PRBool rw)
{
    if ((dbType == SDB_LEGACY) || (dbType == SDB_MULTIACCESS)) {
	return sftkdbCall_ReleaseSecmodDBData(appName, filename, dbname, 
					  moduleSpecList, rw);
    }
    if (moduleSpecList) {
	sftkdb_releaseSpecList(moduleSpecList);
    }
    return SECSuccess;
}





SECStatus
sftkdb_DeleteSecmodDB(SDBType dbType, const char *appName, 
		      const char *filename, const char *dbname, 
		      char *args, PRBool rw)
{
    
    FILE *fd = NULL;
    FILE *fd2 = NULL;
    char line[MAX_LINE_LENGTH];
    char *dbname2 = NULL;
    char *block = NULL;
    char *name = NULL;
    char *lib = NULL;
    int name_len, lib_len;
    PRBool skip = PR_FALSE;
    PRBool found = PR_FALSE;

    if ((dbType == SDB_LEGACY) || (dbType == SDB_MULTIACCESS)) {
	return sftkdbCall_DeleteSecmodDB(appName, filename, dbname, args, rw);
    }

    if (!rw) {
	return SECFailure;
    }

    dbname2 = strdup(dbname);
    if (dbname2 == NULL) goto loser;
    dbname2[strlen(dbname)-1]++;

    
    fd = fopen(dbname, "r");
    if (fd == NULL) goto loser;
#ifdef WINCE
    fd2 = fopen(dbname2, "w+");
#else
    fd2 = lfopen(dbname2, "w+", O_CREAT|O_RDWR|O_TRUNC);
#endif
    if (fd2 == NULL) goto loser;

    name = sftk_argGetParamValue("name",args);
    if (name) {
	name_len = PORT_Strlen(name);
    }
    lib = sftk_argGetParamValue("library",args);
    if (lib) {
	lib_len = PORT_Strlen(lib);
    }


    



    
    block = NULL;
    skip = PR_FALSE;
    while (fgets(line, sizeof(line), fd) != NULL) { 
	
	if (*line != '\n') {
	    
	    if (skip) {
		continue;
	    }
	    

	    if (!found && ((name && (PORT_Strncasecmp(line,"name=",5) == 0) &&
		 (PORT_Strncmp(line+5,name,name_len) == 0))  ||
	        (lib && (PORT_Strncasecmp(line,"library=",8) == 0) &&
		 (PORT_Strncmp(line+8,lib,lib_len) == 0)))) {

		
		PORT_Free(block);
		block=NULL;
		
		skip = PR_TRUE;
		
		found =PR_TRUE;
		continue;
	    }
	    
	    block = sftkdb_DupCat(block,line);
	    continue;
	}
	

	if (block) {
	    fwrite(block, PORT_Strlen(block), 1, fd2);
	    PORT_Free(block);
	    block = NULL;
	}
	
	if (!skip) {
	    fputs(line,fd2);
	}
	
	skip = PR_FALSE;
    } 
    fclose(fd);
    fclose(fd2);
    if (found) {
	
	PR_Delete(dbname);
	PR_Rename(dbname2,dbname);
    } else {
	PR_Delete(dbname2);
    }
    PORT_Free(dbname2);
    PORT_Free(lib);
    PORT_Free(name);
    return SECSuccess;

loser:
    if (fd != NULL) {
	fclose(fd);
    }
    if (fd2 != NULL) {
	fclose(fd2);
    }
    if (dbname2) {
	PR_Delete(dbname2);
	PORT_Free(dbname2);
    }
    PORT_Free(lib);
    PORT_Free(name);
    return SECFailure;
}




SECStatus
sftkdb_AddSecmodDB(SDBType dbType, const char *appName, 
		   const char *filename, const char *dbname, 
		   char *module, PRBool rw)
{
    FILE *fd = NULL;
    char *block = NULL;
    PRBool libFound = PR_FALSE;

    if ((dbType == SDB_LEGACY) || (dbType == SDB_MULTIACCESS)) {
	return sftkdbCall_AddSecmodDB(appName, filename, dbname, module, rw);
    }

    
    if (!rw) {
	return SECFailure;
    }

    
    (void) sftkdb_DeleteSecmodDB(dbType, appName, filename, dbname, module, rw);

#ifdef WINCE
    fd = fopen(dbname, "a+");
#else
    fd = lfopen(dbname, "a+", O_CREAT|O_RDWR|O_APPEND);
#endif
    if (fd == NULL) {
	return SECFailure;
    }
    module = sftk_argStrip(module);
    while (*module) {
	int count;
	char *keyEnd = PORT_Strchr(module,'=');
	char *value;

	if (PORT_Strncmp(module, "library=", 8) == 0) {
	   libFound=PR_TRUE;
	}
	if (keyEnd == NULL) {
	    block = sftkdb_DupCat(block, module);
	    break;
	}
	block = sftkdb_DupnCat(block, module, keyEnd-module+1);
	if (block == NULL) { goto loser; }
	value = sftk_argFetchValue(&keyEnd[1], &count);
	if (value) {
	    block = sftkdb_DupCat(block, sftk_argStrip(value));
	    PORT_Free(value);
	}
	if (block == NULL) { goto loser; }
	block = sftkdb_DupnCat(block, "\n", 1);
	module = keyEnd + 1 + count;
	module = sftk_argStrip(module);
    }
    if (block) {
	if (!libFound) {
	    fprintf(fd,"library=\n");
	}
	fwrite(block, PORT_Strlen(block), 1, fd);
	fprintf(fd,"\n");
	PORT_Free(block);
	block = NULL;
    }
    fclose(fd);
    return SECSuccess;

loser:
    PORT_Free(block);
    fclose(fd);
    return SECFailure;
}
  

