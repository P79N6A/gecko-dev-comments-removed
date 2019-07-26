



#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nss.h"
#include "secutil.h"
#include "pk11pub.h"
#include "cert.h"

typedef struct commandDescriptStr {
    int required;
    char *arg;
    char *des;
} commandDescript;

enum optionNames {
    opt_liborder = 0, 
    opt_mainDB, 
    opt_lib1DB,
    opt_lib2DB,
    opt_mainRO,
    opt_lib1RO,
    opt_lib2RO,
    opt_mainCMD,
    opt_lib1CMD,
    opt_lib2CMD,
    opt_mainTokNam,
    opt_lib1TokNam,
    opt_lib2TokNam,
    opt_oldStyle,
    opt_verbose,
    opt_summary,
    opt_help,
    opt_last
};


static const
secuCommandFlag options_init[] =
{
   {   'o', PR_TRUE, "1M2zmi", PR_TRUE,  "order" },
   {     'd', PR_TRUE,     0,    PR_FALSE, "main_db" },
   {     '1', PR_TRUE,     0,    PR_FALSE, "lib1_db" },
   {     '2', PR_TRUE,     0,    PR_FALSE, "lib2_db" },
   {     'r', PR_FALSE,    0,    PR_FALSE, "main_readonly" },
   {      0,  PR_FALSE,    0,    PR_FALSE, "lib1_readonly" },
   {      0,  PR_FALSE,    0,    PR_FALSE, "lib2_readonly" },
   {    'c', PR_TRUE,     0,    PR_FALSE, "main_command" },
   {     0,  PR_TRUE,     0,    PR_FALSE, "lib1_command" },
   {     0,  PR_TRUE,     0,    PR_FALSE, "lib2_command" },
   { 't', PR_TRUE,     0,    PR_FALSE, "main_token_name" },
   {  0,  PR_TRUE,     0,    PR_FALSE, "lib1_token_name" },
   {  0,  PR_TRUE,     0,    PR_FALSE, "lib2_token_name" },
   {   's', PR_FALSE,    0,    PR_FALSE, "oldStype" },
   {    'v', PR_FALSE,    0,    PR_FALSE, "verbose" },
   {    'z', PR_FALSE,    0,    PR_FALSE, "summary" },
   {       'h', PR_FALSE,    0,    PR_FALSE, "help" }
};

static const
commandDescript options_des[] =
{
   {   PR_FALSE, "initOrder", 
	" Specifies the order of NSS initialization and shutdown. Order is\n"
	" given as a string where each character represents either an init or\n"
	" a shutdown of the main program or one of the 2 test libraries\n"
	" (library 1 and library 2). The valid characters are as follows:\n"
	"   M Init the main program\n   1 Init library 1\n"
	"   2 Init library 2\n"
	"   m Shutdown the main program\n   i Shutdown library 1\n"
	"   z Shutdown library 2\n" },
   {    PR_TRUE, "nss_db",
	" Specified the directory to open the nss database for the main\n" 
	" program. Must be specified if \"M\" is given in the order string\n"},
   {    PR_FALSE, "nss_db",
	" Specified the directory to open the nss database for library 1.\n" 
	" Must be specified if \"1\" is given in the order string\n"},
   {    PR_FALSE, "nss_db",
	" Specified the directory to open the nss database for library 2.\n" 
	" Must be specified if \"2\" is given in the order string\n"},
   {    PR_FALSE,    NULL,
	" Open the main program's database read only.\n" },
   {    PR_FALSE,    NULL,
	" Open library 1's database read only.\n" },
   {    PR_FALSE,    NULL,
	" Open library 2's database read only.\n" },
   {   PR_FALSE,  "nss_command",
	" Specifies the NSS command to execute in the main program.\n"
	" Valid commands are: \n"
	"   key_slot, list_slots, list_certs, add_cert, none.\n"
	" Default is \"none\".\n" },
   {   PR_FALSE,  "nss_command",
	" Specifies the NSS command to execute in library 1.\n" },
   {   PR_FALSE,  "nss_command",
	" Specifies the NSS command to execute in library 2.\n" },
   { PR_FALSE,  "token_name",
	" Specifies the name of PKCS11 token for the main program's "
	"database.\n" },
   { PR_FALSE,  "token_name",
	" Specifies the name of PKCS11 token for library 1's database.\n" },
   { PR_FALSE,  "token_name",
	" Specifies the name of PKCS11 token for library 2's database.\n" },
   {  PR_FALSE,   NULL,
	" Use NSS_Shutdown rather than NSS_ShutdownContext in the main\n"
	" program.\n" },
   {   PR_FALSE,   NULL,
	" Noisily output status to standard error\n" },
   {  PR_FALSE,  NULL, 
	"report a summary of the test results\n" },
   {  PR_FALSE,   NULL, " give this message\n" }
}; 




static void
short_help(const char *prog)
{
    int count = opt_last;
    int i,words_found;

    

    PR_STATIC_ASSERT(sizeof(options_init)/sizeof(secuCommandFlag) == opt_last);
    PR_STATIC_ASSERT(sizeof(options_init)/sizeof(secuCommandFlag) == 
		     sizeof(options_des)/sizeof(commandDescript));

    
    fprintf(stderr,"usage: %s ",prog);
    for (i=0, words_found=0; i < count; i++) {
	if (!options_des[i].required) {
	    fprintf(stderr,"[");
	}
	if (options_init[i].longform) {
	    fprintf(stderr, "--%s", options_init[i].longform);
	    words_found++;
	} else {
	    fprintf(stderr, "-%c", options_init[i].flag);
	}
	if (options_init[i].needsArg) {
	    if (options_des[i].arg) {
		fprintf(stderr," %s",options_des[i].arg);
	    } else {
		fprintf(stderr," arg");
	    }
	    words_found++;
	}
	if (!options_des[i].required) {
	    fprintf(stderr,"]");
	}
	if (i < count-1 ) {
	    if (words_found >= 5) {
 		fprintf(stderr,"\n      ");
		words_found=0;
	    } else {
		fprintf(stderr," ");
	    }
	}
    }
    fprintf(stderr,"\n");
}




static void
long_help(const char *prog)
{
    int i;
    int count = opt_last;

    short_help(prog);
    
    fprintf(stderr,"\n");
    for (i=0; i < count; i++) {
	fprintf(stderr,"        ");
	if (options_init[i].flag) {
	    fprintf(stderr, "-%c", options_init[i].flag);
	    if (options_init[i].longform) {
		fprintf(stderr,",");
	    }
	}
	if (options_init[i].longform) {
	    fprintf(stderr,"--%s", options_init[i].longform);
	}
	if (options_init[i].needsArg) {
	    if (options_des[i].arg) {
		fprintf(stderr," %s",options_des[i].arg);
	    } else {
		fprintf(stderr," arg");
	    }
	    if (options_init[i].arg) {
		fprintf(stderr," (default = \"%s\")",options_init[i].arg);
	    }
	}
	fprintf(stderr,"\n%s",options_des[i].des);
    }
}




struct bufferData {
   char * data;		
   char * next;		
   int  len;		
};



static struct bufferData buffer= { NULL, NULL, 0 };

#define CHUNK_SIZE 1000





static void
initBuffer(void)
{
   buffer.data = PORT_Alloc(CHUNK_SIZE);
   if (!buffer.data) {
	return;
   }
   buffer.next = buffer.data;
   buffer.len = CHUNK_SIZE;
}






static void
growBuffer(void)
{
   char *new = PORT_Realloc(buffer.data, buffer.len + CHUNK_SIZE);
   if (!new) {
	buffer.data[buffer.len-2] = 'D'; 
	
	buffer.next = buffer.data + (buffer.len -1);
	return;
   }
   buffer.next = new + (buffer.next-buffer.data);
   buffer.data = new;
   buffer.len += CHUNK_SIZE;
}




static void
appendLabel(char label)
{
    if (!buffer.data) {
	return;
    }

    *buffer.next++ = label;
    if (buffer.data+buffer.len >= buffer.next) {
	growBuffer();
    }
}




static void
appendString(char *string)
{
    if (!buffer.data) {
	return;
    }

    appendLabel('<');
    while (*string) {
	appendLabel(*string++);
    }
    appendLabel('>');
}




static void
appendBool(PRBool bool)
{
    if (!buffer.data) {
	return;
    }

    if (bool) {
	appendLabel('t');
    } else {
	appendLabel('f');
    }
}




static void
appendHex(unsigned char nibble)
{
    if (nibble <= 9) {
	appendLabel('0'+nibble);
    } else {
	appendLabel('a'+nibble-10);
    }
}




static void
appendItem(SECItem *item)
{
    int i;

    if (!buffer.data) {
	return;
    }

    appendLabel(':');
    for (i=0; i < item->len; i++) {
	unsigned char byte=item->data[i];
	appendHex(byte >> 4);
	appendHex(byte & 0xf);
	appendLabel(':');
    }
}





static void
appendInt(unsigned int value)
{
    int i;

    if (!buffer.data) {
	return;
    }

    appendLabel('0');
    appendLabel('x');
    value = value & 0xffffffff; 
    for (i=0; i < 8; i++) {
	appendHex(value >> 28 );
	value = value << 4;
    }
}


static void
appendFlags(unsigned int flag)
{
  char trust[10];
  char *cp=trust;

  trust[0] = 0;
  printflags(trust, flag);
  while (*cp) {
    appendLabel(*cp++);
  }
}





static void
dumpBuffer(void)
{
    if (!buffer.data) {
	return;
    }

    appendLabel(0); 
    printf("\nresult=%s\n",buffer.data);
    PORT_Free(buffer.data);
    buffer.data = buffer.next = NULL;
    buffer.len = 0;
}





static void
usage(const char *prog)
{
    short_help(prog);
    dumpBuffer();
    exit(1);
}




static void
usage_long(const char *prog)
{
    long_help(prog);
    dumpBuffer();
    exit(1);
}

static const char *
bool2String(PRBool bool) 
{ 
    return bool ? "true" : "false";
}




void
print_slot(PK11SlotInfo *slot, int log)
{
    if (log) {
	fprintf(stderr, "* Name=%s Token_Name=%s present=%s, ro=%s *\n",
		PK11_GetSlotName(slot), PK11_GetTokenName(slot),
		bool2String(PK11_IsPresent(slot)), 
		bool2String(PK11_IsReadOnly(slot)));
    }
    appendLabel('S');
    appendString(PK11_GetTokenName(slot));
    appendBool(PK11_IsPresent(slot));
    appendBool(PK11_IsReadOnly(slot));
}




void
do_list_slots(const char *progName, int log)
{
   PK11SlotList *list;
   PK11SlotListElement *le;

   list= PK11_GetAllTokens(CKM_INVALID_MECHANISM, PR_FALSE, PR_FALSE, NULL);
   if (list == NULL) {
	fprintf(stderr,"ERROR: no tokens found %s\n", 
		SECU_Strerror(PORT_GetError()));
	appendLabel('S');
	appendString("none");
	return;
   }

   for (le= PK11_GetFirstSafe(list); le; 
				le = PK11_GetNextSafe(list,le,PR_TRUE)) {
	print_slot(le->slot, log);
   }
   PK11_FreeSlotList(list);
}

static PRBool
sort_CN(CERTCertificate *certa, CERTCertificate *certb, void *arg)
{
    char *commonNameA, *commonNameB;
    int ret;

    commonNameA = CERT_GetCommonName(&certa->subject);
    commonNameB = CERT_GetCommonName(&certb->subject);

    if (commonNameA == NULL) {
	PORT_Free(commonNameB);
	return PR_TRUE;
    }
    if (commonNameB == NULL) {
	PORT_Free(commonNameA);
	return PR_FALSE;
    }
    ret = PORT_Strcmp(commonNameA,commonNameB);
    PORT_Free(commonNameA);
    PORT_Free(commonNameB);
    return (ret < 0) ? PR_TRUE: PR_FALSE;
}




void
do_list_certs(const char *progName, int log)
{
   CERTCertList *list;
   CERTCertList *sorted;
   CERTCertListNode *node;
   int i;

   list = PK11_ListCerts(PK11CertListUnique, NULL);
   if (list == NULL) {
	fprintf(stderr,"ERROR: no certs found %s\n", 
		SECU_Strerror(PORT_GetError()));
	appendLabel('C');
	appendString("none");
	return;
   }

   sorted = CERT_NewCertList();
   if (sorted == NULL) {
	fprintf(stderr,"ERROR: no certs found %s\n", 
		SECU_Strerror(PORT_GetError()));
	appendLabel('C');
	appendLabel('E');
	appendInt(PORT_GetError());
	return;
   }

   
   for (node = CERT_LIST_HEAD(list); !CERT_LIST_END(node,list); 
				node = CERT_LIST_NEXT(node)) {
	CERT_AddCertToListSorted(sorted, node->cert, sort_CN, NULL);
   }
    

   for (node = CERT_LIST_HEAD(sorted); !CERT_LIST_END(node,sorted); 
				node = CERT_LIST_NEXT(node)) {
	CERTCertificate *cert = node->cert;
	char *commonName;

	SECU_PrintCertNickname(node, stderr);
	if (log) {
	    fprintf(stderr, "*	Slot=%s*\n", cert->slot ?
		 PK11_GetTokenName(cert->slot) : "none");
	    fprintf(stderr, "*	Nickname=%s*\n", cert->nickname);
	    fprintf(stderr, "*	Subject=<%s>*\n", cert->subjectName);
	    fprintf(stderr, "*	Issuer=<%s>*\n", cert->issuerName);
	    fprintf(stderr, "*	SN=");
	    for (i=0; i < cert->serialNumber.len; i++) {
		if (i!=0) fprintf(stderr,":");
		fprintf(stderr, "%02x",cert->serialNumber.data[0]);
	    }
	    fprintf(stderr," *\n");
	}
	appendLabel('C');
	commonName = CERT_GetCommonName(&cert->subject);
	appendString(commonName?commonName:"*NoName*");
	PORT_Free(commonName);
	if (cert->trust) {
	    appendFlags(cert->trust->sslFlags);
	    appendFlags(cert->trust->emailFlags);
	    appendFlags(cert->trust->objectSigningFlags);
	}
   }
   CERT_DestroyCertList(list);

}




void
do_add_cert(const char *progName, int log)
{
  PORT_Assert( 0);
}




void
do_key_slot(const char *progName, int log)
{
   PK11SlotInfo *slot = PK11_GetInternalKeySlot();
   if (!slot) {
	fprintf(stderr,"ERROR: no internal key slot found %s\n", 
		SECU_Strerror(PORT_GetError()));
	appendLabel('K');
	appendLabel('S');
	appendString("none");
   }
   print_slot(slot, log);
   PK11_FreeSlot(slot);
}




void
do_command(const char *label, int initialized, secuCommandFlag *command, 
	   const char *progName, int log)
{
   char * command_string;
   if (!initialized) {
	return;
   }

   if (command->activated) {
	command_string = command->arg;
   } else {
	command_string = "none";
   }

   if (log) {
	fprintf(stderr, "*Executing nss command \"%s\" for %s*\n", 
						command_string,label);
   }

   
   if (PORT_Strcasecmp(command_string, "list_slots") == 0) {
	do_list_slots(progName, log);
   } else if (PORT_Strcasecmp(command_string, "list_certs") == 0) {
	do_list_certs(progName, log);
   } else if (PORT_Strcasecmp(command_string, "add_cert") == 0) {
	do_add_cert(progName, log);
   } else if (PORT_Strcasecmp(command_string, "key_slot") == 0) {
	do_key_slot(progName, log);
   } else if (PORT_Strcasecmp(command_string, "none") != 0) {
	fprintf(stderr, ">> Unknown command (%s)\n", command_string);
	appendLabel('E');
	appendString("bc");
	usage_long(progName);
   }

}






static int main_initialized;
static int lib1_initialized;
static int lib2_initialized;

void
main_Init(secuCommandFlag *db, secuCommandFlag *tokNam,
	  int readOnly, const char *progName, int log)
{
    SECStatus rv;
    if (log) {
	fprintf(stderr,"*NSS_Init for the main program*\n");
    }
    appendLabel('M');
    if (!db->activated) { 
	fprintf(stderr, ">> No main_db has been specified\n");
	usage(progName);
    }
    if (main_initialized) {
	fprintf(stderr,"Warning: Second initialization of Main\n");
	appendLabel('E');
	appendString("2M");
    }
    if (tokNam->activated) {
	PK11_ConfigurePKCS11(NULL, NULL, NULL, tokNam->arg,
			 NULL, NULL, NULL, NULL, 0, 0);
    }
    rv = NSS_Initialize(db->arg, "", "", "", 
		NSS_INIT_NOROOTINIT|(readOnly?NSS_INIT_READONLY:0));
    if (rv != SECSuccess) {
	appendLabel('E');
	appendInt(PORT_GetError());
	fprintf(stderr,">> %s\n", SECU_Strerror(PORT_GetError()));
	dumpBuffer();
	exit(1);
    }
    main_initialized = 1;
}

void
main_Do(secuCommandFlag *command, const char *progName, int log) 
{
    do_command("main", main_initialized, command, progName, log);
}

void
main_Shutdown(int old_style, const char *progName, int log)
{
    SECStatus rv;
    appendLabel('N');
    if (log) {
	fprintf(stderr,"*NSS_Shutdown for the main program*\n");
    }
    if (!main_initialized) {
	fprintf(stderr,"Warning: Main shutdown without corresponding init\n");
    }
    if (old_style) {
	rv = NSS_Shutdown();
    } else {
	rv = NSS_ShutdownContext(NULL);
    }
    fprintf(stderr, "Shutdown main state = %d\n", rv);
    if (rv != SECSuccess) {
	appendLabel('E');
	appendInt(PORT_GetError());
	fprintf(stderr,"ERROR: %s\n", SECU_Strerror(PORT_GetError()));
    }
    main_initialized = 0;
}


NSSInitContext *
lib_Init(const char *lableString, char label, int initialized, 
	 secuCommandFlag *db, secuCommandFlag *tokNam, int readonly, 
	 const char *progName, int log) 
{
    NSSInitContext *ctxt;
    NSSInitParameters initStrings;
    NSSInitParameters *initStringPtr = NULL;

    appendLabel(label);
    if (log) {
	fprintf(stderr,"*NSS_Init for %s*\n", lableString);
    }

    if (!db->activated) { 
	fprintf(stderr, ">> No %s_db has been specified\n", lableString);
	usage(progName);
    }
    if (initialized) {
	fprintf(stderr,"Warning: Second initialization of %s\n", lableString);
    }
    if (tokNam->activated) {
	PORT_Memset(&initStrings, 0, sizeof(initStrings));
	initStrings.length = sizeof(initStrings);
	initStrings.dbTokenDescription = tokNam->arg;
	initStringPtr = &initStrings;
    }
    ctxt = NSS_InitContext(db->arg, "", "", "", initStringPtr,
		NSS_INIT_NOROOTINIT|(readonly?NSS_INIT_READONLY:0));
    if (ctxt == NULL) {
	appendLabel('E');
	appendInt(PORT_GetError());
	fprintf(stderr,">> %s\n",SECU_Strerror(PORT_GetError()));
	dumpBuffer();
	exit(1);
    }
    return ctxt;
}


void
lib_Shutdown(const char *labelString, char label, NSSInitContext *ctx, 
	     int initialize, const char *progName, int log)
{
    SECStatus rv;
    appendLabel(label);
    if (log) {
	fprintf(stderr,"*NSS_Shutdown for %s\n*", labelString);
    }
    if (!initialize) {
	fprintf(stderr,"Warning: %s shutdown without corresponding init\n",
		 labelString);
    }
    rv = NSS_ShutdownContext(ctx);
    fprintf(stderr, "Shutdown %s state = %d\n", labelString, rv);
    if (rv != SECSuccess) {
	appendLabel('E');
	appendInt(PORT_GetError());
	fprintf(stderr,"ERROR: %s\n", SECU_Strerror(PORT_GetError()));
    }
}


static NSSInitContext *lib1_context;
static NSSInitContext *lib2_context;
void
lib1_Init(secuCommandFlag *db, secuCommandFlag *tokNam,
	  int readOnly, const char *progName, int log)
{
    lib1_context = lib_Init("lib1", '1', lib1_initialized, db, tokNam,
			     readOnly, progName, log);
    lib1_initialized = 1;
}

void
lib2_Init(secuCommandFlag *db, secuCommandFlag *tokNam,
	  int readOnly, const char *progName, int log) 
{
    lib2_context = lib_Init("lib2", '2', lib2_initialized,
			    db, tokNam, readOnly, progName, log);
    lib2_initialized = 1;
}

void    
lib1_Do(secuCommandFlag *command, const char *progName, int log) 
{
    do_command("lib1", lib1_initialized, command, progName, log);
}

void
lib2_Do(secuCommandFlag *command, const char *progName, int log) 
{
    do_command("lib2", lib2_initialized, command, progName, log);
}

void
lib1_Shutdown(const char *progName, int log) 
{
     lib_Shutdown("lib1", 'I', lib1_context, lib1_initialized, progName, log);
     lib1_initialized = 0;
     

}

void
lib2_Shutdown(const char *progName, int log) 
{
    lib_Shutdown("lib2", 'Z', lib2_context, lib2_initialized, progName, log);
    lib2_initialized = 0;
    

}

int
main(int argc, char **argv)
{
   SECStatus rv;
   secuCommand libinit;
   char *progName;
   char *order;
   secuCommandFlag *options;
   int log = 0;

   progName = strrchr(argv[0], '/');
   progName = progName ? progName+1 : argv[0];

   libinit.numCommands = 0;
   libinit.commands = 0; 
   libinit.numOptions = opt_last;
   options = (secuCommandFlag *)PORT_Alloc(sizeof(options_init));
   if (options == NULL) {
	fprintf(stderr, ">> %s:Not enough free memory to run command\n",
		progName);
	exit(1);
   }
   PORT_Memcpy(options, options_init, sizeof(options_init));
   libinit.options = options;

   rv = SECU_ParseCommandLine(argc, argv, progName, & libinit);
   if (rv != SECSuccess) {
	usage(progName);
   }

   if (libinit.options[opt_help].activated) {
	long_help(progName);
	exit (0);
   }

   log = libinit.options[opt_verbose].activated;
   if (libinit.options[opt_summary].activated) {
	initBuffer();
   }

   order = libinit.options[opt_liborder].arg;
   if (!order) {
	usage(progName);
   }

   if (log) {
	fprintf(stderr,"* initializing with order \"%s\"*\n", order);
   }

   for (;*order; order++) {
	switch (*order) {
	case 'M':
	    main_Init(&libinit.options[opt_mainDB],
		      &libinit.options[opt_mainTokNam],
		       libinit.options[opt_mainRO].activated,
		       progName, log);
	    break;
	case '1':
	    lib1_Init(&libinit.options[opt_lib1DB],
		      &libinit.options[opt_lib1TokNam],
		       libinit.options[opt_lib1RO].activated,
		        progName,log);
	    break;
	case '2':
	    lib2_Init(&libinit.options[opt_lib2DB],
		      &libinit.options[opt_lib2TokNam],
		       libinit.options[opt_lib2RO].activated,
		       progName,log);
	    break;
	case 'm':
	    main_Shutdown(libinit.options[opt_oldStyle].activated, 
			  progName, log);
	    break;
	case 'i':
	    lib1_Shutdown(progName, log);
	    break;
	case 'z':
	    lib2_Shutdown(progName, log);
	    break;
	default:
	    fprintf(stderr,">> Unknown init/shutdown command \"%c\"", *order);
	    usage_long(progName);
	}
	main_Do(&libinit.options[opt_mainCMD], progName, log);
	lib1_Do(&libinit.options[opt_lib1CMD], progName, log);
	lib2_Do(&libinit.options[opt_lib2CMD], progName, log);
   }

   if (NSS_IsInitialized()) {
	appendLabel('X');
	fprintf(stderr, "Warning: NSS is initialized\n");
   }
   dumpBuffer();

   exit(0);
}

