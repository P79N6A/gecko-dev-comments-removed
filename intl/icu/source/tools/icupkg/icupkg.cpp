




























#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "cstring.h"
#include "toolutil.h"
#include "uoptions.h"
#include "uparse.h"
#include "filestrm.h"
#include "package.h"
#include "pkg_icu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

U_NAMESPACE_USE





#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))



static void
printUsage(const char *pname, UBool isHelp) {
    FILE *where=isHelp ? stdout : stderr;

    fprintf(where,
            "%csage: %s [-h|-?|--help ] [-tl|-tb|-te] [-c] [-C comment]\n"
            "\t[-a list] [-r list] [-x list] [-l [-o outputListFileName]]\n"
            "\t[-s path] [-d path] [-w] [-m mode]\n"
            "\t[--auto_toc_prefix] [--auto_toc_prefix_with_type] [--toc_prefix]\n"
            "\tinfilename [outfilename]\n",
            isHelp ? 'U' : 'u', pname);
    if(isHelp) {
        fprintf(where,
            "\n"
            "Read the input ICU .dat package file, modify it according to the options,\n"
            "swap it to the desired platform properties (charset & endianness),\n"
            "and optionally write the resulting ICU .dat package to the output file.\n"
            "Items are removed, then added, then extracted and listed.\n"
            "An ICU .dat package is written if items are removed or added,\n"
            "or if the input and output filenames differ,\n"
            "or if the --writepkg (-w) option is set.\n");
        fprintf(where,
            "\n"
            "If the input filename is \"new\" then an empty package is created.\n"
            "If the output filename is missing, then it is automatically generated\n"
            "from the input filename: If the input filename ends with an l, b, or e\n"
            "matching its platform properties, then the output filename will\n"
            "contain the letter from the -t (--type) option.\n");
        fprintf(where,
            "\n"
            "This tool can also be used to just swap a single ICU data file, replacing the\n"
            "former icuswap tool. For this mode, provide the infilename (and optional\n"
            "outfilename) for a non-package ICU data file.\n"
            "Allowed options include -t, -w, -s and -d.\n"
            "The filenames can be absolute, or relative to the source/dest dir paths.\n"
            "Other options are not allowed in this mode.\n");
        fprintf(where,
            "\n"
            "Options:\n"
            "\t(Only the last occurrence of an option is used.)\n"
            "\n"
            "\t-h or -? or --help    print this message and exit\n");
        fprintf(where,
            "\n"
            "\t-tl or --type l   output for little-endian/ASCII charset family\n"
            "\t-tb or --type b   output for big-endian/ASCII charset family\n"
            "\t-te or --type e   output for big-endian/EBCDIC charset family\n"
            "\t                  The output type defaults to the input type.\n"
            "\n"
            "\t-c or --copyright include the ICU copyright notice\n"
            "\t-C comment or --comment comment   include a comment string\n");
        fprintf(where,
            "\n"
            "\t-a list or --add list      add items to the package\n"
            "\t-r list or --remove list   remove items from the package\n"
            "\t-x list or --extract list  extract items from the package\n"
            "\tThe list can be a single item's filename,\n"
            "\tor a .txt filename with a list of item filenames,\n"
            "\tor an ICU .dat package filename.\n");
        fprintf(where,
            "\n"
            "\t-w or --writepkg  write the output package even if no items are removed\n"
            "\t                  or added (e.g., for only swapping the data)\n");
        fprintf(where,
            "\n"
            "\t-m mode or --matchmode mode  set the matching mode for item names with\n"
            "\t                             wildcards\n"
            "\t        noslash: the '*' wildcard does not match the '/' tree separator\n");
        fprintf(where,
            "\n"
            "\tIn the .dat package, the Table of Contents (ToC) contains an entry\n"
            "\tfor each item of the form prefix/tree/itemname .\n"
            "\tThe prefix normally matches the package basename, and icupkg checks that,\n"
            "\tbut this is not necessary when ICU need not find and load the package by filename.\n"
            "\tICU package names end with the platform type letter, and thus differ\n"
            "\tbetween platform types. This is not required for user data packages.\n");
        fprintf(where,
            "\n"
            "\t--auto_toc_prefix            automatic ToC entries prefix\n"
            "\t                             Uses the prefix of the first entry of the\n"
            "\t                             input package, rather than its basename.\n"
            "\t                             Requires a non-empty input package.\n"
            "\t--auto_toc_prefix_with_type  auto_toc_prefix + adjust platform type\n"
            "\t                             Same as auto_toc_prefix but also checks that\n"
            "\t                             the prefix ends with the input platform\n"
            "\t                             type letter, and modifies it to the output\n"
            "\t                             platform type letter.\n"
            "\t                At most one of the auto_toc_prefix options\n"
            "\t                can be used at a time.\n"
            "\t--toc_prefix prefix          ToC prefix to be used in the output package\n"
            "\t                             Overrides the package basename\n"
            "\t                             and --auto_toc_prefix.\n"
            "\t                             Cannot be combined with --auto_toc_prefix_with_type.\n");
        




        fprintf(where,
            "\n"
            "\tList file syntax: Items are listed on one or more lines and separated\n"
            "\tby whitespace (space+tab).\n"
            "\tComments begin with # and are ignored. Empty lines are ignored.\n"
            "\tLines where the first non-whitespace character is one of %s\n"
            "\tare also ignored, to reserve for future syntax.\n",
            U_PKG_RESERVED_CHARS);
        fprintf(where,
            "\tItems for removal or extraction may contain a single '*' wildcard\n"
            "\tcharacter. The '*' matches zero or more characters.\n"
            "\tIf --matchmode noslash (-m noslash) is set, then the '*'\n"
            "\tdoes not match '/'.\n");
        fprintf(where,
            "\n"
            "\tItems must be listed relative to the package, and the --sourcedir or\n"
            "\tthe --destdir path will be prepended.\n"
            "\tThe paths are only prepended to item filenames while adding or\n"
            "\textracting items, not to ICU .dat package or list filenames.\n"
            "\t\n"
            "\tPaths may contain '/' instead of the platform's\n"
            "\tfile separator character, and are converted as appropriate.\n");
        fprintf(where,
            "\n"
            "\t-s path or --sourcedir path  directory for the --add items\n"
            "\t-d path or --destdir path    directory for the --extract items\n"
            "\n"
            "\t-l or --list                 list the package items\n"
            "\t                             (after modifying the package)\n"
            "\t                             to stdout or to output list file\n"
            "\t-o path or --outlist path    path/filename for the --list output\n");
    }
}

static UOption options[]={
    UOPTION_HELP_H,
    UOPTION_HELP_QUESTION_MARK,
    UOPTION_DEF("type", 't', UOPT_REQUIRES_ARG),

    UOPTION_COPYRIGHT,
    UOPTION_DEF("comment", 'C', UOPT_REQUIRES_ARG),

    UOPTION_SOURCEDIR,
    UOPTION_DESTDIR,

    UOPTION_DEF("writepkg", 'w', UOPT_NO_ARG),

    UOPTION_DEF("matchmode", 'm', UOPT_REQUIRES_ARG),

    UOPTION_DEF("add", 'a', UOPT_REQUIRES_ARG),
    UOPTION_DEF("remove", 'r', UOPT_REQUIRES_ARG),
    UOPTION_DEF("extract", 'x', UOPT_REQUIRES_ARG),

    UOPTION_DEF("list", 'l', UOPT_NO_ARG),
    UOPTION_DEF("outlist", 'o', UOPT_REQUIRES_ARG),

    UOPTION_DEF("auto_toc_prefix", '\1', UOPT_NO_ARG),
    UOPTION_DEF("auto_toc_prefix_with_type", '\1', UOPT_NO_ARG),
    UOPTION_DEF("toc_prefix", '\1', UOPT_REQUIRES_ARG)
};

enum {
    OPT_HELP_H,
    OPT_HELP_QUESTION_MARK,
    OPT_OUT_TYPE,

    OPT_COPYRIGHT,
    OPT_COMMENT,

    OPT_SOURCEDIR,
    OPT_DESTDIR,

    OPT_WRITEPKG,

    OPT_MATCHMODE,

    OPT_ADD_LIST,
    OPT_REMOVE_LIST,
    OPT_EXTRACT_LIST,

    OPT_LIST_ITEMS,
    OPT_LIST_FILE,

    OPT_AUTO_TOC_PREFIX,
    OPT_AUTO_TOC_PREFIX_WITH_TYPE,
    OPT_TOC_PREFIX,

    OPT_COUNT
};

static UBool
isPackageName(const char *filename) {
    int32_t len;

    len=(int32_t)strlen(filename)-4; 
    return (UBool)(len>0 && 0==strcmp(filename+len, ".dat"));
}




int _CRT_glob = 0;

extern int
main(int argc, char *argv[]) {
    const char *pname, *sourcePath, *destPath, *inFilename, *outFilename, *outComment;
    char outType;
    UBool isHelp, isModified, isPackage;
    int result = 0;

    Package *pkg, *listPkg, *addListPkg;

    U_MAIN_INIT_ARGS(argc, argv);

    
    pname=findBasename(argv[0]);

    argc=u_parseArgs(argc, argv, LENGTHOF(options), options);
    isHelp=options[OPT_HELP_H].doesOccur || options[OPT_HELP_QUESTION_MARK].doesOccur;
    if(isHelp) {
        printUsage(pname, TRUE);
        return U_ZERO_ERROR;
    }

    pkg=new Package;
    if(pkg==NULL) {
        fprintf(stderr, "icupkg: not enough memory\n");
        return U_MEMORY_ALLOCATION_ERROR;
    }
    isModified=FALSE;

    int autoPrefix=0;
    if(options[OPT_AUTO_TOC_PREFIX].doesOccur) {
        pkg->setAutoPrefix();
        ++autoPrefix;
    }
    if(options[OPT_AUTO_TOC_PREFIX_WITH_TYPE].doesOccur) {
        if(options[OPT_TOC_PREFIX].doesOccur) {
            fprintf(stderr, "icupkg: --auto_toc_prefix_with_type and also --toc_prefix\n");
            printUsage(pname, FALSE);
            return U_ILLEGAL_ARGUMENT_ERROR;
        }
        pkg->setAutoPrefixWithType();
        ++autoPrefix;
    }
    if(argc<2 || 3<argc || autoPrefix>1) {
        printUsage(pname, FALSE);
        return U_ILLEGAL_ARGUMENT_ERROR;
    }

    if(options[OPT_SOURCEDIR].doesOccur) {
        sourcePath=options[OPT_SOURCEDIR].value;
    } else {
        
        sourcePath=NULL;
    }
    if(options[OPT_DESTDIR].doesOccur) {
        destPath=options[OPT_DESTDIR].value;
    } else {
        
        destPath=NULL;
    }

    if(0==strcmp(argv[1], "new")) {
        if(autoPrefix) {
            fprintf(stderr, "icupkg: --auto_toc_prefix[_with_type] but no input package\n");
            printUsage(pname, FALSE);
            return U_ILLEGAL_ARGUMENT_ERROR;
        }
        inFilename=NULL;
        isPackage=TRUE;
    } else {
        inFilename=argv[1];
        if(isPackageName(inFilename)) {
            pkg->readPackage(inFilename);
            isPackage=TRUE;
        } else {
            
            pkg->addFile(sourcePath, inFilename);
            isPackage=FALSE;
        }
    }

    if(argc>=3) {
        outFilename=argv[2];
        if(0!=strcmp(argv[1], argv[2])) {
            isModified=TRUE;
        }
    } else if(isPackage) {
        outFilename=NULL;
    } else  {
        outFilename=inFilename;
        isModified=(UBool)(sourcePath!=destPath);
    }

    
    if(options[OPT_OUT_TYPE].doesOccur) {
        const char *type=options[OPT_OUT_TYPE].value;
        if(type[0]==0 || type[1]!=0) {
            
            printUsage(pname, FALSE);
            return U_ILLEGAL_ARGUMENT_ERROR;
        }
        outType=type[0];
        switch(outType) {
        case 'l':
        case 'b':
        case 'e':
            break;
        default:
            printUsage(pname, FALSE);
            return U_ILLEGAL_ARGUMENT_ERROR;
        }

        





        isModified|=(UBool)(!isPackage || outType!=pkg->getInType());
    } else if(isPackage) {
        outType=pkg->getInType(); 
    } else  {
        outType=0; 
    }

    if(options[OPT_WRITEPKG].doesOccur) {
        isModified=TRUE;
    }

    if(!isPackage) {
        



        if( options[OPT_COMMENT].doesOccur ||
            options[OPT_COPYRIGHT].doesOccur ||
            options[OPT_MATCHMODE].doesOccur ||
            options[OPT_REMOVE_LIST].doesOccur ||
            options[OPT_ADD_LIST].doesOccur ||
            options[OPT_EXTRACT_LIST].doesOccur ||
            options[OPT_LIST_ITEMS].doesOccur
        ) {
            printUsage(pname, FALSE);
            return U_ILLEGAL_ARGUMENT_ERROR;
        }
        if(isModified) {
            pkg->extractItem(destPath, outFilename, 0, outType);
        }

        delete pkg;
        return result;
    }

    

    if(options[OPT_COMMENT].doesOccur) {
        outComment=options[OPT_COMMENT].value;
    } else if(options[OPT_COPYRIGHT].doesOccur) {
        outComment=U_COPYRIGHT_STRING;
    } else {
        outComment=NULL;
    }

    if(options[OPT_MATCHMODE].doesOccur) {
        if(0==strcmp(options[OPT_MATCHMODE].value, "noslash")) {
            pkg->setMatchMode(Package::MATCH_NOSLASH);
        } else {
            printUsage(pname, FALSE);
            return U_ILLEGAL_ARGUMENT_ERROR;
        }
    }

    
    if(options[OPT_REMOVE_LIST].doesOccur) {
        listPkg=new Package();
        if(listPkg==NULL) {
            fprintf(stderr, "icupkg: not enough memory\n");
            exit(U_MEMORY_ALLOCATION_ERROR);
        }
        if(readList(NULL, options[OPT_REMOVE_LIST].value, FALSE, listPkg)) {
            pkg->removeItems(*listPkg);
            delete listPkg;
            isModified=TRUE;
        } else {
            printUsage(pname, FALSE);
            return U_ILLEGAL_ARGUMENT_ERROR;
        }
    }

    




    addListPkg=NULL;
    if(options[OPT_ADD_LIST].doesOccur) {
        addListPkg=new Package();
        if(addListPkg==NULL) {
            fprintf(stderr, "icupkg: not enough memory\n");
            exit(U_MEMORY_ALLOCATION_ERROR);
        }
        if(readList(sourcePath, options[OPT_ADD_LIST].value, TRUE, addListPkg)) {
            pkg->addItems(*addListPkg);
            
            isModified=TRUE;
        } else {
            printUsage(pname, FALSE);
            return U_ILLEGAL_ARGUMENT_ERROR;
        }
    }

    
    if(options[OPT_EXTRACT_LIST].doesOccur) {
        listPkg=new Package();
        if(listPkg==NULL) {
            fprintf(stderr, "icupkg: not enough memory\n");
            exit(U_MEMORY_ALLOCATION_ERROR);
        }
        if(readList(NULL, options[OPT_EXTRACT_LIST].value, FALSE, listPkg)) {
            pkg->extractItems(destPath, *listPkg, outType);
            delete listPkg;
        } else {
            printUsage(pname, FALSE);
            return U_ILLEGAL_ARGUMENT_ERROR;
        }
    }

    
    if(options[OPT_LIST_ITEMS].doesOccur) {
        int32_t i;
        if (options[OPT_LIST_FILE].doesOccur) {
            FileStream *out;
            out = T_FileStream_open(options[OPT_LIST_FILE].value, "w");
            if (out != NULL) {
                for(i=0; i<pkg->getItemCount(); ++i) {
                    T_FileStream_writeLine(out, pkg->getItem(i)->name);
                    T_FileStream_writeLine(out, "\n");
                }
                T_FileStream_close(out);
            } else {
                return U_ILLEGAL_ARGUMENT_ERROR;
            }
        } else {
            for(i=0; i<pkg->getItemCount(); ++i) {
                fprintf(stdout, "%s\n", pkg->getItem(i)->name);
            }
        }
    }

    
    if(!pkg->checkDependencies()) {
        
        return U_MISSING_RESOURCE_ERROR;
    }

    
    if(isModified) {
        char outFilenameBuffer[1024]; 

        if(outFilename==NULL || outFilename[0]==0) {
            if(inFilename==NULL || inFilename[0]==0) {
                fprintf(stderr, "icupkg: unable to auto-generate an output filename if there is no input filename\n");
                exit(U_ILLEGAL_ARGUMENT_ERROR);
            }

            





            char suffix[6]="?.dat";
            char *s;

            suffix[0]=pkg->getInType();
            strcpy(outFilenameBuffer, inFilename);
            s=strchr(outFilenameBuffer, 0);
            if((s-outFilenameBuffer)>5 && 0==memcmp(s-5, suffix, 5)) {
                *(s-5)=outType;
            }
            outFilename=outFilenameBuffer;
        }
        if(options[OPT_TOC_PREFIX].doesOccur) {
            pkg->setPrefix(options[OPT_TOC_PREFIX].value);
        }
        result = writePackageDatFile(outFilename, outComment, NULL, NULL, pkg, outType);
    }

    delete addListPkg;
    delete pkg;
    return result;
}









