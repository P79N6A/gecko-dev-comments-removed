








































#include "xpidl.h"

static ModeData modes[] = {
    {"header",  "Generate C++ header",         "h",    xpidl_header_dispatch},
    {"typelib", "Generate XPConnect typelib",  "xpt",  xpidl_typelib_dispatch},
    {"doc",     "Generate HTML documentation", "html", xpidl_doc_dispatch},
    {"java",    "Generate Java interface",     "java", xpidl_java_dispatch},
    {0,         0,                             0,      0}
};

static ModeData *
FindMode(char *mode)
{
    int i;
    for (i = 0; modes[i].mode; i++) {
        if (!strcmp(modes[i].mode, mode))
            return &modes[i];
    }
    return NULL;
}

gboolean enable_debug               = FALSE;
gboolean enable_warnings            = FALSE;
gboolean verbose_mode               = FALSE;
gboolean emit_typelib_annotations   = FALSE;
gboolean explicit_output_filename   = FALSE;


PRUint8  major_version              = XPT_MAJOR_VERSION;
PRUint8  minor_version              = XPT_MINOR_VERSION;

static char xpidl_usage_str[] =
"Usage: %s -m mode [-w] [-v] [-t version number]\n"
"          [-I path] [-o basename | -e filename.ext]\n"
"          [-p Java package] filename.idl\n"
"       -a emit annotations to typelib\n"
"       -w turn on warnings (recommended)\n"
"       -v verbose mode (NYI)\n"
"       -t create a typelib of a specific version number\n"
"       -I add entry to start of include path for ``#include \"nsIThing.idl\"''\n"
"       -o use basename (e.g. ``/tmp/nsIThing'') for output\n"
"       -e use explicit output filename\n"
"       -p specify package name (Java mode only)\n"
"       -m specify output mode:\n";

static void
xpidl_usage(int argc, char *argv[])
{
    int i;
    fprintf(stderr, xpidl_usage_str, argv[0]);
    for (i = 0; modes[i].mode; i++) {
        fprintf(stderr, "          %-12s  %-30s (.%s)\n", modes[i].mode,
                modes[i].modeInfo, modes[i].suffix);
    }
}

#if defined(XP_MAC) && defined(XPIDL_PLUGIN)
#define main xpidl_main
int xpidl_main(int argc, char *argv[]);
#endif

int main(int argc, char *argv[])
{
    int i;
    IncludePathEntry *inc, *inc_head, **inc_tail;
    char *file_basename = NULL, *package = NULL;
    ModeData *mode = NULL;
    gboolean create_old_typelib = FALSE;

    


    inc_head = xpidl_malloc(sizeof *inc);
#ifndef XP_MAC
    inc_head->directory = ".";
#else
    inc_head->directory = "";
#endif
    inc_head->next = NULL;
    inc_tail = &inc_head->next;

    for (i = 1; i < argc; i++) {
        if (argv[i][0] != '-')
            break;
        switch (argv[i][1]) {
          case '-':
            argc++;             
            
          case 0:               
            goto done_options;
          case 'a':
            emit_typelib_annotations = TRUE;
            break;
          case 'w':
            enable_warnings = TRUE;
            break;
          case 'v':
            verbose_mode = TRUE;
            break;
          case 't':
          {
            


            const gchar* typelib_version_string = NULL;

            



            if (i + 1 == argc) {
                fprintf(stderr, "ERROR: missing version number after -t\n");
                xpidl_usage(argc, argv);
                return 1;
            }

            
            if (create_old_typelib) {
                fprintf(stderr,
                        "ERROR: -t argument used twice. "
                        "Cannot specify more than one version\n");
                xpidl_usage(argc, argv);
                return 1;
            }

            




            switch (XPT_ParseVersionString(argv[++i], &major_version, 
                                           &minor_version)) {
              case XPT_VERSION_CURRENT:
                break; 
              case XPT_VERSION_OLD: 
                create_old_typelib = TRUE;
                break; 
              case XPT_VERSION_UNSUPPORTED: 
                fprintf(stderr, "ERROR: version \"%s\" not supported.\n", 
                        argv[i]);
                xpidl_usage(argc, argv);
                return 1;          
              case XPT_VERSION_UNKNOWN: 
              default:
                fprintf(stderr, "ERROR: version \"%s\" not recognised.\n", 
                        argv[i]);
                xpidl_usage(argc, argv);
                return 1;          
            }
            break;
          }
          case 'I':
            if (argv[i][2] == '\0' && i == argc) {
                fputs("ERROR: missing path after -I\n", stderr);
                xpidl_usage(argc, argv);
                return 1;
            }
            inc = xpidl_malloc(sizeof *inc);
            if (argv[i][2] == '\0') {
                
                inc->directory = argv[++i];
            } else {
                
                inc->directory = argv[i] + 2;
            }
#ifdef DEBUG_shaver_includes
            fprintf(stderr, "adding %s to include path\n", inc->directory);
#endif
            inc->next = NULL;
            *inc_tail = inc;
            inc_tail = &inc->next;
            break;
          case 'o':
            if (i == argc) {
                fprintf(stderr, "ERROR: missing basename after -o\n");
                xpidl_usage(argc, argv);
                return 1;
            }
            file_basename = argv[++i];
            explicit_output_filename = FALSE;
            break;
          case 'e':
            if (i == argc) {
                fprintf(stderr, "ERROR: missing basename after -e\n");
                xpidl_usage(argc, argv);
                return 1;
            }
            file_basename = argv[++i];
            explicit_output_filename = TRUE;
            break;
          case 'm':
            if (i + 1 == argc) {
                fprintf(stderr, "ERROR: missing modename after -m\n");
                xpidl_usage(argc, argv);
                return 1;
            }
            if (mode) {
                fprintf(stderr,
                        "ERROR: must specify exactly one mode "
                        "(first \"%s\", now \"%s\")\n", mode->mode,
                        argv[i + 1]);
                xpidl_usage(argc, argv);
                return 1;
            }
            mode = FindMode(argv[++i]);
            if (!mode) {
                fprintf(stderr, "ERROR: unknown mode \"%s\"\n", argv[i]);
                xpidl_usage(argc, argv);
                return 1;
            }
            break;
          case 'p':
            if (i + 1 == argc) {
                fprintf(stderr, "ERROR: missing package name after -p\n");
                xpidl_usage(argc, argv);
                return 1;
            }
            if (package) {
                fprintf(stderr,
                        "ERROR: must specify exactly one package "
                        "(first \"%s\", now \"%s\")\n", package,
                        argv[i + 1]);
                xpidl_usage(argc, argv);
                return 1;
            }
            package = argv[++i];
            if (!package) {
                fprintf(stderr, "ERROR: unknown package \"%s\"\n", argv[i]);
                xpidl_usage(argc, argv);
                return 1;
            }
            break;
          default:
            fprintf(stderr, "unknown option %s\n", argv[i]);
            xpidl_usage(argc, argv);
            return 1;
        }
    }
 done_options:
    if (!mode) {
        fprintf(stderr, "ERROR: must specify output mode\n");
        xpidl_usage(argc, argv);
        return 1;
    }
    if (argc != i + 1) {
        fprintf(stderr, "ERROR: extra arguments after input file\n");
    }

    



    if (xpidl_process_idl(argv[i], inc_head, file_basename, package, mode))
        return 0;

    return 1;
}
