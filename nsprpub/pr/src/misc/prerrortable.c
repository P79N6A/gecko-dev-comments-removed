

























#include <string.h>
#include <assert.h>
#include <errno.h>
#include "prmem.h"
#include "prerror.h"

#define	ERRCODE_RANGE	8	/* # of bits to shift table number */
#define	BITS_PER_CHAR	6	/* # bits to shift per character in name */

#ifdef NEED_SYS_ERRLIST
extern char const * const sys_errlist[];
extern const int sys_nerr;
#endif


struct PRErrorTableList {
    struct PRErrorTableList *next;
    const struct PRErrorTable *table;
    struct PRErrorCallbackTablePrivate *table_private;
};
static struct PRErrorTableList * Table_List = (struct PRErrorTableList *) NULL;


static const char * default_languages[] = { "i-default", "en", 0 };
static const char * const * callback_languages = default_languages;


static struct PRErrorCallbackPrivate *callback_private = 0;
static PRErrorCallbackLookupFn *callback_lookup = 0;
static PRErrorCallbackNewTableFn *callback_newtable = 0;


static const char char_set[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_";

static const char *
error_table_name (PRErrorCode num)
{
    static char buf[6];	

    long ch;
    int i;
    char *p;

    
    p = buf;
    num >>= ERRCODE_RANGE;
    
    num &= 077777777;
    
    for (i = 4; i >= 0; i--) {
	ch = (num >> BITS_PER_CHAR * i) & ((1 << BITS_PER_CHAR) - 1);
	if (ch != 0)
	    *p++ = char_set[ch-1];
    }
    *p = '\0';
    return(buf);
}

PR_IMPLEMENT(const char *)
PR_ErrorToString(PRErrorCode code, PRLanguageCode language)
{
    


    static char buffer[25];

    const char *msg;
    int offset;
    PRErrorCode table_num;
    struct PRErrorTableList *et;
    int started = 0;
    char *cp;

    for (et = Table_List; et; et = et->next) {
	if (et->table->base <= code &&
	    et->table->base + et->table->n_msgs > code) {
	    
	    if (callback_lookup) {
		msg = callback_lookup(code, language, et->table,
		    callback_private, et->table_private);
		if (msg) return msg;
	    }
    
	    return(et->table->msgs[code - et->table->base].en_text);
	}
    }

    if (code >= 0 && code < 256) {
	return strerror(code);
    }

    offset = (int) (code & ((1<<ERRCODE_RANGE)-1));
    table_num = code - offset;
    strcpy (buffer, "Unknown code ");
    if (table_num) {
	strcat(buffer, error_table_name (table_num));
	strcat(buffer, " ");
    }
    for (cp = buffer; *cp; cp++)
	;
    if (offset >= 100) {
	*cp++ = (char)('0' + offset / 100);
	offset %= 100;
	started++;
    }
    if (started || offset >= 10) {
	*cp++ = (char)('0' + offset / 10);
	offset %= 10;
    }
    *cp++ = (char)('0' + offset);
    *cp = '\0';
    return(buffer);
}

PR_IMPLEMENT(const char *)
PR_ErrorToName(PRErrorCode code)
{
    struct PRErrorTableList *et;

    for (et = Table_List; et; et = et->next) {
	if (et->table->base <= code &&
	    et->table->base + et->table->n_msgs > code) {
	    
	    return(et->table->msgs[code - et->table->base].name);
	}
    }

    return 0;
}

PR_IMPLEMENT(const char * const *)
PR_ErrorLanguages(void)
{
    return callback_languages;
}

PR_IMPLEMENT(PRErrorCode)
PR_ErrorInstallTable(const struct PRErrorTable *table)
{
    struct PRErrorTableList * new_et;

    new_et = (struct PRErrorTableList *)
					PR_Malloc(sizeof(struct PRErrorTableList));
    if (!new_et)
	return errno;	
    new_et->table = table;
    if (callback_newtable) {
	new_et->table_private = callback_newtable(table, callback_private);
    } else {
	new_et->table_private = 0;
    }
    new_et->next = Table_List;
    Table_List = new_et;
    return 0;
}

PR_IMPLEMENT(void)
PR_ErrorInstallCallback(const char * const * languages,
		       PRErrorCallbackLookupFn *lookup, 
		       PRErrorCallbackNewTableFn *newtable,
		       struct PRErrorCallbackPrivate *cb_private)
{
    struct PRErrorTableList *et;

    assert(strcmp(languages[0], "i-default") == 0);
    assert(strcmp(languages[1], "en") == 0);
    
    callback_languages = languages;
    callback_lookup = lookup;
    callback_newtable = newtable;
    callback_private = cb_private;

    if (callback_newtable) {
	for (et = Table_List; et; et = et->next) {
	    et->table_private = callback_newtable(et->table, callback_private);
	}
    }
}
