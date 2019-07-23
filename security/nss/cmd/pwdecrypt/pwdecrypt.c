









































#include "nspr.h"
#include "string.h"
#include "nss.h"
#include "secutil.h"
#include "cert.h"
#include "pk11func.h"
#include "nssb64.h"

#include "plgetopt.h"
#include "pk11sdr.h"

#define DEFAULT_VALUE "Test"

static void
synopsis (char *program_name)
{
    PRFileDesc *pr_stderr;

    pr_stderr = PR_STDERR;
    PR_fprintf (pr_stderr,
	"Usage:\t%s [-i <input-file>] [-o <output-file>] [-d <dir>]\n"
        "      \t[-l logfile] [-p pwd] [-f pwfile]\n", program_name);
}


static void
short_usage (char *program_name)
{
    PR_fprintf (PR_STDERR,
		"Type %s -H for more detailed descriptions\n",
		program_name);
    synopsis (program_name);
}


static void
long_usage (char *program_name)
{
    PRFileDesc *pr_stderr;

    pr_stderr = PR_STDERR;
    synopsis (program_name);
    PR_fprintf (pr_stderr, "\nDecode encrypted passwords (and other data).\n");
    PR_fprintf (pr_stderr, 
	"This program reads in standard configuration files looking\n"
	"for base 64 encoded data. Data that looks like it's base 64 encode\n"
	"is decoded an passed to the NSS SDR code. If the decode and decrypt\n"
	"is successful, then decrypted data is outputted in place of the\n"
	"original base 64 data. If the decode or decrypt fails, the original\n"
	"data is written and the reason for failure is logged to the \n"
	"optional logfile.\n");
    PR_fprintf (pr_stderr,
		"  %-13s Read stream including encrypted data from "
	        "\"read_file\"\n",
		"-i read_file");
    PR_fprintf (pr_stderr,
		"  %-13s Write results to \"write_file\"\n",
		"-o write_file");
    PR_fprintf (pr_stderr,
		"  %-13s Find security databases in \"dbdir\"\n",
		"-d dbdir");
    PR_fprintf (pr_stderr,
		"  %-13s Log failed decrypt/decode attempts to \"log_file\"\n",
		"-l log_file");
    PR_fprintf (pr_stderr,
		"  %-13s Token password\n",
		"-p pwd");
    PR_fprintf (pr_stderr,
		"  %-13s Password file\n",
		"-f pwfile");
}




static unsigned char b64[256] = {
        0,      0,      0,      0,      0,      0,      0,      0,
        0,      0,      0,      0,      0,      0,      0,      0,
        0,      0,      0,      0,      0,      0,      0,      0,
        0,      0,      0,      0,      0,      0,      0,      0,
        0,      0,      0,      0,      0,      0,      0,      0,
        0,      0,      0,      1,      0,      0,      0,      1,
        1,      1,      1,      1,      1,      1,      1,      1,
        1,      1,      0,      0,      0,      0,      0,      0,
        0,      1,      1,      1,      1,      1,      1,      1,
        1,      1,      1,      1,      1,      1,      1,      1,
        1,      1,      1,      1,      1,      1,      1,      1,
        1,      1,      1,      0,      0,      0,      0,      0,
        0,      1,      1,      1,      1,      1,      1,      1,
        1,      1,      1,      1,      1,      1,      1,      1,
        1,      1,      1,      1,      1,      1,      1,      1,
        1,      1,      1,      0,      0,      0,      0,      0,
        0,      0,      0,      0,      0,      0,      0,      0
};

enum {
   false = 0,
   true = 1
} bool;

int
isatobchar(int c) { return b64[c] != 0; }


#define MAX_STRING 256
int
getData(FILE *inFile,char **inString) {
    int len = 0;
    int space = MAX_STRING;
    int oneequal = false;
    int c;
    char *string = (char *) malloc(space);

    string[len++]='M';

    while ((c = getc(inFile)) != EOF) {
	if (len >= space) {
	    char *newString;

	    space *= 2;
	    newString = (char *)realloc(string,space);
	    if (newString == NULL) {
		ungetc(c,inFile);
		break;
	    }
	    string = newString;
	}
	string[len++] = c;
	if (!isatobchar(c)) {
	   if (c == '=') {
		if (oneequal) {
		    break;
		}
		oneequal = true;
		continue;
	   } else {
	       ungetc(c,inFile);
	       len--;
	       break;
	   }
	}
	if (oneequal) {
	   ungetc(c,inFile);
	   len--;
	   break;
	}
    }
    if (len >= space) {
	space += 2;
	string = (char *)realloc(string,space);
    }
    string[len++] = 0;
    *inString = string;
    return true;
}

int
main (int argc, char **argv)
{
    int		 retval = 0;  
    SECStatus	 rv;
    PLOptState	*optstate;
    char	*program_name;
    char  *input_file = NULL; 	
    char  *output_file = NULL;	
    char  *log_file = NULL;	
    FILE	*inFile = stdin;
    FILE	*outFile = stdout;
    FILE	*logFile = NULL;
    PLOptStatus optstatus;
    SECItem	result;
    int		c;
    secuPWData  pwdata = { PW_NONE, NULL };

    result.data = 0;

    program_name = PL_strrchr(argv[0], '/');
    program_name = program_name ? (program_name + 1) : argv[0];

    optstate = PL_CreateOptState (argc, argv, "Hd:f:i:o:l:p:?");
    if (optstate == NULL) {
	SECU_PrintError (program_name, "PL_CreateOptState failed");
	return 1;
    }

    while ((optstatus = PL_GetNextOpt(optstate)) == PL_OPT_OK) {
	switch (optstate->option) {
	  case '?':
	    short_usage (program_name);
	    return 1;

	  case 'H':
	    long_usage (program_name);
	    return 1;

	  case 'd':
	    SECU_ConfigDirectory(optstate->value);
	    break;

          case 'i':
            input_file = PL_strdup(optstate->value);
            break;

          case 'o':
            output_file = PL_strdup(optstate->value);
            break;

          case 'l':
            log_file = PL_strdup(optstate->value);
            break;

          case 'f':
            pwdata.source = PW_FROMFILE;
            pwdata.data = PL_strdup(optstate->value);
            break;

          case 'p':
            pwdata.source = PW_PLAINTEXT;
            pwdata.data = PL_strdup(optstate->value);
            break;

	}
    }
    PL_DestroyOptState(optstate);
    if (optstatus == PL_OPT_BAD) {
	short_usage (program_name);
	return 1;
    }

    if (input_file) {
      inFile = fopen(input_file,"r");
      if (inFile == NULL) {
	perror(input_file);
	return 1;
      }
      PR_Free(input_file);
    }
    if (output_file) {
      outFile = fopen(output_file,"w+");
      if (outFile == NULL) {
	perror(output_file);
	return 1;
      }
      PR_Free(output_file);
    }
    if (log_file) {
      logFile = fopen(log_file,"w+");
      if (logFile == NULL) {
	perror(log_file);
	return 1;
      }
      PR_Free(log_file);
    }

    


    PK11_SetPasswordFunc(SECU_GetModulePassword);
    rv = NSS_Init(SECU_ConfigDirectory(NULL));
    if (rv != SECSuccess) {
	SECU_PrintError (program_name, "NSS_Init failed");
	retval = 1;
	goto prdone;
    }

    



    while ((c = getc(inFile)) != EOF) {
	if (c == 'M') {
	   char *dataString = NULL;
	   SECItem *inText;

	   rv = getData(inFile, &dataString);
	   if (!rv) {
		fputs(dataString,outFile);
		free(dataString);
		continue;
	   }
	   inText = NSSBase64_DecodeBuffer(NULL, NULL, dataString,
							strlen(dataString));
	   if ((inText == NULL) || (inText->len == 0)) {
		if (logFile) {
		    fprintf(logFile,"Base 64 decode failed on <%s>\n",
								dataString);
		    fprintf(logFile," Error %x: %s\n",PORT_GetError(),
			SECU_Strerror(PORT_GetError()));
		}
		fputs(dataString,outFile);
		free(dataString);
		continue;
	   }
	   result.data = NULL;
	   result.len  = 0;
	   rv = PK11SDR_Decrypt(inText, &result, &pwdata);
	   SECITEM_FreeItem(inText, PR_TRUE);
	   if (rv != SECSuccess) {
		if (logFile) {
		    fprintf(logFile,"SDR decrypt failed on <%s>\n",
								dataString);
		    fprintf(logFile," Error %x: %s\n",PORT_GetError(),
			SECU_Strerror(PORT_GetError()));
		}
		fputs(dataString,outFile);
		free(dataString);
		SECITEM_ZfreeItem(&result, PR_FALSE);
		continue;
	   }
	   
	   fprintf(outFile, "%.*s", result.len, result.data);
	   SECITEM_ZfreeItem(&result, PR_FALSE);
         } else {
	   putc(c,outFile);
         }
    }

    fclose(outFile);
    fclose(inFile);
    if (logFile) {
	fclose(logFile);
    }

    if (NSS_Shutdown() != SECSuccess) {
	SECU_PrintError (program_name, "NSS_Shutdown failed");
       exit(1);
    }

prdone:
    PR_Cleanup ();
    return retval;
}
