





































#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "VerReg.h"
#include "NSReg.h"

extern char *errstr(REGERR err);
extern int DumpTree(void);


int error(char *func, int err)
{
	if (err == REGERR_OK)
	{
    	printf("\t%s -- OK\n", func);
	}
	else
	{
		printf("\t%s -- %s\n", func, errstr(err));
	}

	return err;

}	

static char  *GetNextWord(char *cmd, char *buf)
{
	
	if (!cmd || !buf)
		return 0;
	while (*cmd && *cmd != ',')
		*buf++ = *cmd++;
	*buf = '\0';
	if (*cmd == ',')
	{
		cmd++;
		while(*cmd && *cmd == ' ')
			cmd++;
	}
	return cmd;

}	

static int vr_ParseVersion(char *verstr, VERSION *result)
{

	result->major = result->minor = result->release = result->build = 0;
	result->major = atoi(verstr);
	while (*verstr && *verstr != '.')
		verstr++;
	if (*verstr)
	{
		verstr++;
		result->minor = atoi(verstr);
		while (*verstr && *verstr != '.')
			verstr++;
		if (*verstr)
		{
			verstr++;
			result->release = atoi(verstr);
			while (*verstr && *verstr != '.')
				verstr++;
			if (*verstr)
			{
				verstr++;
				result->build = atoi(verstr);
				while (*verstr && *verstr != '.')
					verstr++;
			}
		}
	}

	return REGERR_OK;

}	


void vCreate(char *cmd)
{

	
	char buf[512];
    
	int flag = 0;
	cmd = GetNextWord(cmd, buf);
    
	error("VR_CreateRegistry", VR_CreateRegistry("Communicator", buf, cmd));

}	



void vFind(char *cmd)
{

	VERSION ver;
	char path[MAXREGPATHLEN];

	if (error("VR_GetVersion", VR_GetVersion(cmd, &ver)) == REGERR_OK)
	{
		if (error("VR_GetPath", VR_GetPath(cmd, sizeof(path), path)) == REGERR_OK)
		{
			printf("%s found: ver=%d.%d.%d.%d, check=0x%04x, path=%s\n", 
				cmd, ver.major, ver.minor, ver.release, ver.build, ver.check,
				path);
			return;
		}
	}

	printf("%s not found.\n", cmd);
	return;

}	


void vHelp(char *cmd)
{

	puts("Enter a command:");
    puts("\tN)ew <dir> [, <ver>] - create a new registry");
    puts("\tA)pp <dir>       - set application directory");
    puts("\tC)lose           - close the registry");
    puts("");
	puts("\tI)nstall <name>, <version>, <path> - install a new component");
	puts("\tR)emove <name>   - deletes a component from the Registry");
    puts("\tX)ists <name>    - checks for existence in registry");
    puts("\tT)est <name>     - validates physical existence");
    puts("\tE)num <name>     - dumps named subtree");
    puts("");
    puts("\tV)ersion <name>  - gets component version");
    puts("\tP)ath <name>     - gets component path");
    puts("\treF)count <name> - gets component refcount");
    puts("\tD)ir <name>      - gets component directory");
    puts("\tSR)efcount <name>- sets component refcount");
    puts("\tSD)ir <name>     - sets component directory");
    puts("");
	puts("\tQ)uit            - end the program");

}	


void vInstall(char *cmd)
{

	char name[MAXREGPATHLEN+1];
	char path[MAXREGPATHLEN+1];
    char ver[MAXREGPATHLEN+1];

    char *pPath, *pVer;

	cmd = GetNextWord(cmd, name);
    cmd = GetNextWord(cmd, ver);
    cmd = GetNextWord(cmd, path);

    pVer  = ( ver[0]  != '*' ) ? ver  : NULL;
    pPath = ( path[0] != '*' ) ? path : NULL;

	error("VR_Install", VR_Install(name, pPath, pVer, FALSE));

}	




			

void interp(void)
{

	char line[256];
	char *p;

	while(1)
	{
		putchar('>');
		putchar(' ');
		fflush(stdin); fflush(stdout); fflush(stderr);
		gets(line);

		
		p = line;
		while (*p && *p!=' ')
			p++;
		if (!*p)
			p = 0;
		else
		{
			while(*p && *p==' ')
				p++;
		}

		switch(toupper(line[0]))
		{
		case 'N':
			vCreate(p);
			break;
        case 'A':
            error("VR_SetRegDirectory", VR_SetRegDirectory(p));
            break;
        case 'C':
            error("VR_Close", VR_Close());
            break;

		case 'I':
			vInstall(p);
			break;
		case 'R':
        	error("VR_Remove", VR_Remove(p));
			break;
        case 'X':
        	error("VR_InRegistry", VR_InRegistry(p));
            break;
        case 'T':
        	error("VR_ValidateComponent", VR_ValidateComponent(p));
            break;

#if  LATER
        case 'E':
            vEnum(p);
            break;

        case 'V':
            vVersion(p);
            break;
        case 'P':
            vPath(p);
            break;
        case 'F':
            vGetRefCount(p);
            break;
        case 'D':
            vGetDir(p);
            break;
        
        case 'S':
            puts("--Unsupported--");
#endif

		case 'H':
		default:
			vHelp(line);
			break;
		case 'Q':
			return;
		}	
	}	

	assert(0);
	return;	

}	


