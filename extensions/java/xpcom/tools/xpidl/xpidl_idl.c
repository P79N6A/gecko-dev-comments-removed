









































#include "xpidl.h"
#include "limits.h"

#ifdef XP_MAC
#include <stat.h>
#endif

static gboolean parsed_empty_file;




gboolean
xpidl_process_node(TreeState *state)
{
    gint type;
    nodeHandler *dispatch, handler;

    XPT_ASSERT(state->tree);
    type = IDL_NODE_TYPE(state->tree);

    if ((dispatch = state->dispatch) && (handler = dispatch[type]))
        return handler(state);
    return TRUE;
}

#if defined(XP_MAC) && defined(XPIDL_PLUGIN)
extern void mac_warning(const char* warning_message);
#endif

static int
msg_callback(int level, int num, int line, const char *file,
             const char *message)
{
    char *warning_message;

    



    if (0 == strcmp(message, "File empty after optimization")) {
        parsed_empty_file = TRUE;
        return 1;
    }

    if (!file)
        file = "<unknown file>";
    warning_message = g_strdup_printf("%s:%d: %s\n", file, line, message);

#if defined(XP_MAC) && defined(XPIDL_PLUGIN)
    mac_warning(warning_message);
#else
    fputs(warning_message, stderr);
#endif

    g_free(warning_message);
    return 1;
}





typedef struct input_data {
    char *filename;             
    unsigned int lineno;        
    char *buf;                  
    char *point;                
    char *max;                  
    struct input_data *next;    
} input_data;





typedef struct input_callback_state {
    struct input_data *input_stack;    
    GHashTable *already_included;   
    IncludePathEntry *include_path; 
    GSList *base_includes;          


} input_callback_state;

static FILE *
fopen_from_includes(const char *filename, const char *mode,
                    IncludePathEntry *include_path)
{
    IncludePathEntry *current_path = include_path;
    char *pathname;
    FILE *inputfile;
    if (!strcmp(filename, "-"))
        return stdin;

    if (filename[0] != '/') {
        while (current_path) {
            pathname = g_strdup_printf("%s" G_DIR_SEPARATOR_S "%s",
                                       current_path->directory, filename);
            if (!pathname)
                return NULL;
            inputfile = fopen(pathname, mode);
            g_free(pathname);
            if (inputfile)
                return inputfile;
            current_path = current_path->next;
        }
    } else {
        inputfile = fopen(filename, mode);
        if (inputfile)
            return inputfile;
    }
    return NULL;
}

#if defined(XP_MAC) && defined(XPIDL_PLUGIN)
extern FILE* mac_fopen(const char* filename, const char *mode);
#endif

static input_data *
new_input_data(const char *filename, IncludePathEntry *include_path)
{
    input_data *new_data;
    FILE *inputfile;
    char *buffer = NULL;
    size_t offset = 0;
    size_t buffer_size;
#ifdef XP_MAC
    size_t i;
#endif

#if defined(XP_MAC) && defined(XPIDL_PLUGIN)
    
    inputfile = fopen(filename, "r");
#elif defined(XP_OS2) || defined(XP_WIN32)
    



    if( filename[1] == ':' )
      inputfile = fopen(filename, "r");
    else
      inputfile = fopen_from_includes(filename, "r", include_path);
#else
    inputfile = fopen_from_includes(filename, "r", include_path);
#endif

    if (!inputfile)
        return NULL;

#ifdef XP_MAC
    {
        struct stat input_stat;
        if (fstat(fileno(inputfile), &input_stat))
            return NULL;
        buffer = malloc(input_stat.st_size + 1);
        if (!buffer)
            return NULL;
        offset = fread(buffer, 1, input_stat.st_size, inputfile);
        if (ferror(inputfile))
            return NULL;
    }
#else
    







    for (buffer_size = 8191; ; buffer_size *= 2) {
        size_t just_read;
        buffer = realloc(buffer, buffer_size + 1); 
        just_read = fread(buffer + offset, 1, buffer_size - offset, inputfile);
        if (ferror(inputfile))
            return NULL;

        if (just_read < buffer_size - offset || just_read == 0) {
            
            offset += just_read;
            break;
        }
        offset += just_read;
    }
#endif

    fclose(inputfile);

#ifdef XP_MAC
    



    for (i = 0; i < offset; i++) {
        if (buffer[i] == '\r')
            buffer[i] = '\n';
    }
#endif

    new_data = xpidl_malloc(sizeof (struct input_data));
    new_data->point = new_data->buf = buffer;
    new_data->max = buffer + offset;
    *new_data->max = '\0';
    new_data->filename = xpidl_strdup(filename);
    
    new_data->lineno = 2;
    new_data->next = NULL;

    return new_data;
}


static int
NextIsRaw(input_data *data, char **startp, int *lenp)
{
    char *end, *start;

    





    if (!(data->point[0] == '%' && data->point[1] == '{'))
        return 0;
        
    start = *startp = data->point;
    
    end = NULL;
    while (start < data->max && (end = strstr(start, "%}"))) {
        if (end[-1] == '\r' ||
            end[-1] == '\n')
            break;
        start = end + 1;
    }

    if (end && start < data->max) {
        *lenp = end - data->point + 2;
        return 1;
    } else {
        const char *filename;
        int lineno;

        IDL_file_get(&filename, &lineno);
        msg_callback(IDL_ERROR, 0, lineno, filename,
                     "unterminated %{ block");
        return -1;
    }
}


static int
NextIsComment(input_data *data, char **startp, int *lenp)
{
    char *end;

    if (!(data->point[0] == '/' && data->point[1] == '*'))
        return 0;

    end = strstr(data->point, "*/");
    *lenp = 0;
    if (end) {
        int skippedLines = 0;
        char *tempPoint;
        
        
        IDL_file_get(NULL,(int *)&data->lineno);

        
        for (tempPoint = data->point; tempPoint < end; tempPoint++) {
            if (*tempPoint == '\n')
                skippedLines++;
        }

        data->lineno += skippedLines;
        IDL_file_set(data->filename, (int)data->lineno);
        
        *startp = end + 2;

        
        if (data->point[2] == '*') {
            
            char t = *(end + 2);
            *(end + 2) = '\0';
            IDL_queue_new_ident_comment(data->point);
            *(end + 2) = t;
        }

        data->point = *startp; 
        return 1;
    } else {
        const char *filename;
        int lineno;

        IDL_file_get(&filename, &lineno);
        msg_callback(IDL_ERROR, 0, lineno, filename,
                     "unterminated comment");
        return -1;
    }
}

static int
NextIsInclude(input_callback_state *callback_state, char **startp,
              int *lenp)
{
    input_data *data = callback_state->input_stack;
    input_data *new_data;
    char *filename, *end;
    const char *scratch;

    
    if (strncmp(data->point, "#include \"", 10)) {
        return 0;
    }
    
    filename = data->point + 10; 
    XPT_ASSERT(filename < data->max);
    end = filename;
    while (end < data->max) {
        if (*end == '\"' || *end == '\n' || *end == '\r')
            break;
        end++;
    }

    if (*end != '\"') {
        





        end = filename;
        while (end < data->max) {
            if (*end == ' ' || *end == '\n' || *end == '\r' || *end == '\t')
                break;
            end++;
        }
        *end = '\0';
        
        
        IDL_file_get(&scratch, (int *)&data->lineno);
        fprintf(stderr,
                "%s:%d: didn't find end of quoted include name \"%s\n",
                scratch, data->lineno, filename);
        return -1;
    }

    *end = '\0';
    *startp = end + 1;

    if (data->next == NULL) {
        






        char *filename_cp = xpidl_strdup(filename);
        
        
        callback_state->base_includes =
            g_slist_append(callback_state->base_includes, filename_cp);
    }

    
    data->point = *startp;

    if (!g_hash_table_lookup(callback_state->already_included, filename)) {
        filename = xpidl_strdup(filename);
        g_hash_table_insert(callback_state->already_included,
                            filename, (void *)TRUE);
        new_data = new_input_data(filename, callback_state->include_path);
        if (!new_data) {
            char *error_message;
            IDL_file_get(&scratch, (int *)&data->lineno);
            error_message =
                g_strdup_printf("can't open included file %s for reading\n",
                                filename);
            msg_callback(IDL_ERROR, 0,
                         data->lineno, scratch, error_message);
            g_free(error_message);
            return -1;
        }

        new_data->next = data;
        
        IDL_inhibit_push();
        IDL_file_get(&scratch, (int *)&data->lineno);
        callback_state->input_stack = new_data;
        IDL_file_set(new_data->filename, (int)new_data->lineno);
    }

    *lenp = 0;               
    return 1;
}    

static void
FindSpecial(input_data *data, char **startp, int *lenp)
{
    char *point = data->point;

    







#define LINE_START(data, point) (point == data->buf ||                       \
                                 (point > data->point &&                     \
                                  (point[-1] == '\r' || point[-1] == '\n')))
                                                 
    while (point < data->max) {
        if (point[0] == '/' && point[1] == '*')
            break;
        if (LINE_START(data, point)) {
            if (point[0] == '%' && point[1] == '{')
                break;
            if (point[0] == '#' && !strncmp(point + 1, "include \"", 9))
                break;
        }
        point++;
    }

#undef LINE_START

    *startp = data->point;
    *lenp = point - data->point;
}


static FILE *tracefile;

static int
input_callback(IDL_input_reason reason, union IDL_input_data *cb_data,
               gpointer user_data)
{
    input_callback_state *callback_state = user_data;
    input_data *data = callback_state->input_stack;
    input_data *new_data = NULL;
    unsigned int len, copy;
    int rv;
    char *start;

    switch(reason) {
      case IDL_INPUT_REASON_INIT:
        if (data == NULL || data->next == NULL) {
            





            


            IncludePathEntry first_entry;

            first_entry.directory = callback_state->include_path->directory;
            first_entry.next = NULL;

            new_data = new_input_data(cb_data->init.filename,
                                               &first_entry);
        } else {
            new_data = new_input_data(cb_data->init.filename,
                                               callback_state->include_path);
        }

        if (!new_data)
            return -1;

        IDL_file_set(new_data->filename, (int)new_data->lineno);
        callback_state->input_stack = new_data;
        return 0;

      case IDL_INPUT_REASON_FILL:
        start = NULL;
        len = 0;

        while (data->point >= data->max) {
            if (!data->next)
                return 0;

            
            callback_state->input_stack = data->next;
            free(data->filename);
            free(data->buf);
            free(data);
            data = callback_state->input_stack;

            IDL_file_set(data->filename, (int)data->lineno);
            IDL_inhibit_pop();
        }
        
        































        




        rv = NextIsRaw(data, &start, (int *)&len);
        if (rv == -1) return -1;
        if (!rv) {
            



            rv = NextIsComment(data, &start, (int *)&len);
            if (rv == -1) return -1;
            if (!rv) {
                




                rv = NextIsInclude(callback_state, &start, (int *)&len);
                if (rv == -1) return -1;
                if (!rv)
                    FindSpecial(data, &start, (int *)&len);
            }
        }

        if (len == 0) {
            






            return input_callback(reason, cb_data, user_data);
        }

        copy = MIN(len, (unsigned int) cb_data->fill.max_size);
        memcpy(cb_data->fill.buffer, start, copy);
        data->point = start + copy;

        if (tracefile)
            fwrite(cb_data->fill.buffer, copy, 1, tracefile);

        return copy;

      case IDL_INPUT_REASON_ABORT:
      case IDL_INPUT_REASON_FINISH:
        while (data != NULL) {
            input_data *next;

            next = data->next;
            free(data->filename);
            free(data->buf);
            free(data);
            data = next;
        }
        return 0;

      default:
        g_error("unknown input reason %d!", reason);
        return -1;
    }
}

static void
free_ghash_key(gpointer key, gpointer value, gpointer user_data)
{
    
    free(key);
}

static void
free_gslist_data(gpointer data, gpointer user_data)
{
    free(data);
}


#ifdef XP_UNIX
#include <unistd.h>
#elif XP_WIN

#endif

int
xpidl_process_idl(char *filename, IncludePathEntry *include_path,
                  char *file_basename, char* package, ModeData *mode)
{
    char *tmp, *outname, *real_outname = NULL;
    IDL_tree top;
    TreeState state;
    int rv;
    input_callback_state callback_state;
    gboolean ok = TRUE;
    backend *emitter;

    memset(&state, 0, sizeof(state));

    callback_state.input_stack = NULL;
    callback_state.base_includes = NULL;
    callback_state.include_path = include_path;
    callback_state.already_included = g_hash_table_new(g_str_hash, g_str_equal);

    if (!callback_state.already_included) {
        fprintf(stderr, "failed to create hashtable.  out of memory?\n");
        return 0;
    }

    state.basename = xpidl_strdup(filename);
    if (package)
      state.package = xpidl_strdup(package);

    
    tmp = strrchr(state.basename, '.');
    if (tmp)
        *tmp = '\0';

    if (!file_basename)
        outname = xpidl_strdup(state.basename);
    else
        outname = xpidl_strdup(file_basename);

    
    g_hash_table_insert(callback_state.already_included,
                        xpidl_strdup(filename), (void *)TRUE);

    parsed_empty_file = FALSE;

    rv = IDL_parse_filename_with_input(filename, input_callback, &callback_state,
                                       msg_callback, &top,
                                       &state.ns,
                                       IDLF_IGNORE_FORWARDS |
                                       IDLF_XPIDL,
                                       enable_warnings ? IDL_WARNING1 :
                                       IDL_ERROR);
    if (parsed_empty_file) {
        





        top = NULL;
        state.ns = NULL;
    } else if (rv != IDL_SUCCESS) {
        if (rv == -1) {
            g_warning("Parse of %s failed: %s", filename, g_strerror(errno));
        } else {
            g_warning("Parse of %s failed", filename);
        }
        return 0;
    }

    state.basename = xpidl_strdup(filename);
    tmp = strrchr(state.basename, '.');
    if (tmp)
        *tmp = '\0';

    
    state.base_includes = callback_state.base_includes;

    emitter = mode->factory();
    state.dispatch = emitter->dispatch_table;

    if (strcmp(outname, "-")) {
        const char *fopen_mode;
        const char *out_basename;

        
        if (explicit_output_filename) {
            real_outname = g_strdup(outname);
        } else {




#if defined(XP_UNIX) || defined(XP_WIN)
            if (!file_basename) {
                out_basename = xpidl_basename(outname);
            } else {
                out_basename = outname;
            }
#else
            out_basename = outname;
#endif
            real_outname = g_strdup_printf("%s.%s", out_basename, mode->suffix);
        }

        
        if (strcmp(mode->mode, "java") == 0)
        {
            state.filename = real_outname;
        } else {
            
            fopen_mode = (strcmp(mode->mode, "typelib")) ? "w" : "wb";
            state.file = fopen(real_outname, fopen_mode);
            if (!state.file) {
                perror("error opening output file");
                return 0;
            }
        }
    } else {
        state.file = stdout;
    }
    state.tree = top;

    if (emitter->emit_prolog)
        emitter->emit_prolog(&state);
    if (state.tree) 
        ok = xpidl_process_node(&state);
    if (emitter->emit_epilog)
        emitter->emit_epilog(&state);

    if (strcmp(mode->mode, "java") != 0)
    {
        if (state.file != stdout)
            fclose(state.file);
    }

    free(state.basename);
    free(state.package);
    free(outname);
    g_hash_table_foreach(callback_state.already_included, free_ghash_key, NULL);
    g_hash_table_destroy(callback_state.already_included);
    g_slist_foreach(callback_state.base_includes, free_gslist_data, NULL);

    if (state.ns)
        IDL_ns_free(state.ns);
    if (top)
        IDL_tree_free(top);

    if (real_outname != NULL) {
        



#if defined(XP_UNIX) || defined(XP_WIN)
        if (!ok)
            unlink(real_outname);
#endif
        g_free(real_outname);
    }

    return ok;
}





void
xpidl_tree_warning(IDL_tree p, int level, const char *fmt, ...)
{
    va_list ap;
    char *msg, *file;
    int lineno;

    
    va_start(ap, fmt);
    msg = g_strdup_vprintf(fmt, ap);

    if (p) {
        file = p->_file;
        lineno = p->_line;
    } else {
        file = NULL;
        lineno = 0;
    }

    
    msg_callback(level, 0, lineno, file, msg);
    g_free(msg);
    va_end(ap);
}
