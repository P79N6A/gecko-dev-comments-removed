



































import sys
import re


versionString = "^#define SQLITE_VERSION\D*(\d+)\.(\d+)\.(\d+)(?:\.(\d+))?\D*"





for line in open(sys.argv[1]):
    result = re.search(versionString, line)
    if result is not None:    
        splitVersion = list(result.groups())
        if splitVersion[3] is None:       
            splitVersion[3:] = ['0']
        print "#define SQLITE_VERSION_MAJOR " + splitVersion[0]
        print "#define SQLITE_VERSION_MINOR " + splitVersion[1]
        print "#define SQLITE_VERSION_PATCH " + splitVersion[2]
        print "#define SQLITE_VERSION_SUBPATCH " + splitVersion[3]
        break
