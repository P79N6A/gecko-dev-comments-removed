




































'''expandlibs-exec.py applies expandlibs rules, and some more (see below) to
a given command line, and executes that command line with the expanded
arguments.

With the --extract argument (useful for e.g. $(AR)), it extracts object files
from static libraries (or use those listed in library descriptors directly).

With the --uselist argument (useful for e.g. $(CC)), it replaces all object
files with a list file. This can be used to avoid limitations in the length
of a command line. The kind of list file format used depends on the
EXPAND_LIBS_LIST_STYLE variable: 'list' for MSVC style lists (@file.list)
or 'linkerscript' for GNU ld linker scripts.
See https://bugzilla.mozilla.org/show_bug.cgi?id=584474#c59 for more details.
'''
from __future__ import with_statement
import sys
import os
from expandlibs import ExpandArgs, relativize
import expandlibs_config as conf
from optparse import OptionParser
import subprocess
import tempfile
import shutil

class ExpandArgsMore(ExpandArgs):
    ''' Meant to be used as 'with ExpandArgsMore(args) as ...: '''
    def __enter__(self):
        self.tmp = []
        return self
        
    def __exit__(self, type, value, tb):
        '''Automatically remove temporary files'''
        for tmp in self.tmp:
            if os.path.isdir(tmp):
                shutil.rmtree(tmp, True)
            else:
                os.remove(tmp)

    def extract(self):
        self[0:] = self._extract(self)

    def _extract(self, args):
        '''When a static library name is found, either extract its contents
        in a temporary directory or use the information found in the
        corresponding lib descriptor.
        '''
        ar_extract = conf.AR_EXTRACT.split()
        newlist = []
        for arg in args:
            if os.path.splitext(arg)[1] == conf.LIB_SUFFIX:
                if os.path.exists(arg + conf.LIBS_DESC_SUFFIX):
                    newlist += self._extract(self._expand_desc(arg))
                elif os.path.exists(arg) and len(ar_extract):
                    tmp = tempfile.mkdtemp(dir=os.curdir)
                    self.tmp.append(tmp)
                    subprocess.call(ar_extract + [os.path.abspath(arg)], cwd=tmp)
                    objs = []
                    for root, dirs, files in os.walk(tmp):
                        objs += [relativize(os.path.join(root, f)) for f in files if os.path.splitext(f)[1] == conf.OBJ_SUFFIX]
                    newlist += objs
                else:
                    newlist += [arg]
            else:
                newlist += [arg]
        return newlist

    def makelist(self):
        '''Replaces object file names with a temporary list file, using a
        list format depending on the EXPAND_LIBS_LIST_STYLE variable
        '''
        objs = [o for o in self if os.path.splitext(o)[1] == conf.OBJ_SUFFIX]
        if not len(objs): return
        fd, tmp = tempfile.mkstemp(suffix=".list",dir=os.curdir)
        if conf.EXPAND_LIBS_LIST_STYLE == "linkerscript":
            content = ["INPUT(%s)\n" % obj for obj in objs]
            ref = tmp
        elif conf.EXPAND_LIBS_LIST_STYLE == "list":
            content = ["%s\n" % obj for obj in objs]
            ref = "@" + tmp
        else:
            os.remove(tmp)
            return
        self.tmp.append(tmp)
        f = os.fdopen(fd, "w")
        f.writelines(content)
        f.close()
        idx = self.index(objs[0])
        newlist = self[0:idx] + [ref] + [item for item in self[idx:] if item not in objs]
        self[0:] = newlist

def main():
    parser = OptionParser()
    parser.add_option("--extract", action="store_true", dest="extract",
        help="when a library has no descriptor file, extract it first, when possible")
    parser.add_option("--uselist", action="store_true", dest="uselist",
        help="use a list file for objects when executing a command")
    parser.add_option("--verbose", action="store_true", dest="verbose",
        help="display executed command and temporary files content")

    (options, args) = parser.parse_args()

    with ExpandArgsMore(args) as args:
        if options.extract:
            args.extract()
        if options.uselist:
            args.makelist()

        if options.verbose:
            print >>sys.stderr, "Executing: " + " ".join(args)
            for tmp in [f for f in args.tmp if os.path.isfile(f)]:
                print >>sys.stderr, tmp + ":"
                with open(tmp) as file:
                    print >>sys.stderr, "".join(["    " + l for l in file.readlines()])
            sys.stderr.flush()
        exit(subprocess.call(args))

if __name__ == '__main__':
    main()
