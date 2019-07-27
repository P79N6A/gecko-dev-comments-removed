



#define __STRINGLIB_INTERNAL__

#include "cpr_types.h"
#include "cpr_stdlib.h"
#include "cpr_stdio.h"
#include "cpr_string.h"
#include "cpr_stddef.h"
#include "cpr_locks.h"
#include "string_lib.h"
#include "phone_debug.h"
#include "debug.h"


#define STRING_SIGNATURE (('S'<<8)|'T')
#define STR_TO_STRUCT(str) ((string_block_t *)((str) - (offsetof(string_block_t,data))))
#define STRUCT_TO_STR(sbt) ((const char *) (sbt)->data)

static string_t empty_str;
static int strlib_is_string(string_t str);














string_t
strlib_malloc (const char *str, int length, const char *fname, int line)
{
    string_block_t *temp;
    int size;

    
    
    
    
    if ((length == LEN_UNKNOWN) || (length < 0)) {
        length = strlen(str);
    }

    size = sizeof(string_block_t) + length + 1;
    temp = (string_block_t *) cpr_malloc(size);

    if (!temp) {
        CSFLogError("src-common",
            "Error: Strlib_Malloc() Failed. Requested Size = %d\n", size);
        return (string_t) 0;
    }

    temp->refcount  = 1;
    temp->length    = (uint16_t) length;
    temp->signature = STRING_SIGNATURE;
    temp->fname     = fname;
    temp->line      = line;
    
    
    sstrncpy(temp->data, str, length + 1);
    temp->data[length] = '\0';

    return STRUCT_TO_STR(temp);
}

















string_t
strlib_copy (string_t str)
{
    string_block_t *temp;

    if (!strlib_is_string(str)) {
        return (NULL);
    }

    temp = STR_TO_STRUCT(str);

    



    if ((temp->refcount < 0xffff) && (str != empty_str)) {
        temp->refcount++;
    }

    return STRUCT_TO_STR(temp);
}













string_t
strlib_update (string_t destination, const char *source,
              const char *calling_fname, int line)
{
    const char *fname = "strlib_udpate";
    string_t ret_str;

    
    if (!destination) {
        
        CSFLogError("src-common", "%s: Destination String is invalid: %s:%d",
                fname, calling_fname, line);
        
        return NULL;
    }

    
    if (!source) {
        
        CSFLogError("src-common", "%s: Source String is invalid: %s:%d", fname,
                calling_fname, line);
        strlib_free(destination);
        return strlib_empty();
    }

    if (source[0] == '\0') {
        
        strlib_free(destination);
        return strlib_empty();
    }

    ret_str = strlib_malloc(source, LEN_UNKNOWN, calling_fname, line);

    if (!ret_str) {
        



        ret_str = destination;
    } else {
        strlib_free(destination);
    }

    return (ret_str);
}














string_t
strlib_append (string_t str, const char *toappend_str,
              const char *fname, int line)
{
    int curlen;
    char *buf;

    
    if (!str) {
        return NULL;
    }

    curlen = strlen(str);
    
    buf = strlib_open(str, curlen + strlen(toappend_str) + 1, fname, line);
    



    if (buf) {
        strcpy(buf + curlen, toappend_str);
        return buf;
    }
    return NULL;
}













void
strlib_free (string_t str)
{
    string_block_t *temp;

    if ((!strlib_is_string(str)) || (str == empty_str)) {
        return;
    }

    temp = STR_TO_STRUCT(str);
    temp->refcount--;
    if (temp->refcount == 0) {
        temp->signature = 0;
        cpr_free(temp);
    }

    return;
}
















char *
strlib_open (string_t str, int length, const char *fname, int line)
{
    char *ret_str;
    string_block_t *temp;

    if (!strlib_is_string(str)) {
        return (NULL);
    }

    temp = STR_TO_STRUCT(str);

    if ((temp->refcount == 1) && (length <= temp->length)) {
        ret_str = (char *) str;
    } else {
        ret_str = (char *) strlib_malloc(str, length, fname, line);
        if (!ret_str) {
            



            ret_str = (char *) str;
        } else {
            strlib_free(str);
        }
    }

    return (ret_str);
}













string_t
strlib_close (char *str)
{
    if (!strlib_is_string(str)) {
        return (NULL);
    }
    return (str);
}

string_t strlib_printf(const char *format, ...) {
    string_t result = strlib_empty();
    va_list args;
    flex_string fs;
    if (format) {
        va_start(args, format);
        flex_string_init(&fs);
        flex_string_vsprintf(&fs, format, args);
        result = strlib_malloc(fs.buffer, -1, __FILE__, __LINE__);
        flex_string_free(&fs);
        va_end(args);
    }
    return result;
}











int
strlib_is_string (string_t str)
{
    string_block_t *temp;

    if (str == NULL) {
        return (0);
    }

    temp = STR_TO_STRUCT(str);

    if (temp->signature == STRING_SIGNATURE) {
        return (1);
    } else {
        CSFLogError("src-common",
          "Strlib Error: strlib_is_tring passed invalid string\n");
        return (0);
    }

}

int
strlib_test_memory_is_string (void *mem)
{
    string_block_t *temp;

    if (!mem) {
        return FALSE;
    }

    temp = (string_block_t *) mem;
    if (temp->signature == STRING_SIGNATURE) {
        return TRUE;
    }
    return FALSE;
}















string_t
strlib_empty (void)
{
    string_block_t *temp;
    static boolean empty_str_init = FALSE;

    if (empty_str_init == FALSE) {
        empty_str = strlib_malloc("", LEN_UNKNOWN, __FILE__, __LINE__);
        temp = STR_TO_STRUCT(empty_str);
        temp->refcount = 0xffff;
        empty_str_init = TRUE;
    }
    return (empty_str);
}

void
strlib_init (void)
{
  (void) strlib_empty(); 
}
