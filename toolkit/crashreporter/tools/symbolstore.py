






















































import sys
import os
import re
import shutil
from optparse import OptionParser



def GetCVSRevision(file):
    """Given a full path to a file, look in CVS/Entries
    for the CVS revision number"""
    (path, filename) = os.path.split(file)
    entries = os.path.join(path, "CVS", "Entries")
    if not os.path.isfile(entries):
        return None
    f = open(entries, "r")
    for line in f:
        parts = line.split("/")
        if len(parts) > 1 and parts[1] == filename:
            return parts[2]
    print >> sys.stderr, "Failed to get CVS Revision for %s" % filename
    return None

def GetCVSRoot(file):
    """Given a full path to a file, look in CVS/Root
    for the CVS Root"""
    (path, filename) = os.path.split(file)
    root = os.path.join(path, "CVS", "Root")
    if not os.path.isfile(root):
        return None
    f = open(root, "r")
    root_name = f.readline().strip()
    f.close()
    parts = root_name.split("@")
    if len(parts) > 1:
        
        return parts[1].replace(":","")
    print >> sys.stderr, "Failed to get CVS Root for %s" % filename
    return None

def GetVCSFilename(file, srcdir):
    """Given a full path to a file, and the top source directory,
    look for version control information about this file, and return
    a specially formatted filename that contains the VCS type,
    VCS location, relative filename, and revision number, formatted like:
    vcs:vcs location:filename:revision
    For example:
    cvs:cvs.mozilla.org/cvsroot:mozilla/browser/app/nsBrowserApp.cpp:1.36"""
    (path, filename) = os.path.split(file)
    if path == '' or filename == '':
        return file
    
    cvsdir = os.path.join(path, "CVS")
    if os.path.isdir(cvsdir):
        rev = GetCVSRevision(file)
        root = GetCVSRoot(file)
        if rev is not None and root is not None:
            if srcdir is not None:
                
                
                
                file = os.path.normpath(file)
                file = file.replace(srcdir, "", 1)
                (head, tail) = os.path.split(srcdir)
                if tail == "":
                    tail = os.path.basename(head)
                file = tail + file
            file = file.replace("\\", "/")
            return "cvs:%s:%s:%s" % (root, file, rev)
    file = file.replace("\\", "/")
    return file

def GetPlatformSpecificDumper(**kwargs):
    """This function simply returns a instance of a subclass of Dumper
    that is appropriate for the current platform."""
    return {'win32': Dumper_Win32,
            'cygwin': Dumper_Win32,
            'linux2': Dumper_Linux,
            'darwin': Dumper_Mac}[sys.platform](**kwargs)

class Dumper:
    """This class can dump symbols from a file with debug info, and
    store the output in a directory structure that is valid for use as
    a Breakpad symbol server.  Requires a path to a dump_syms binary--
    |dump_syms| and a directory to store symbols in--|symbol_path|.
    Optionally takes a list of processor architectures to process from
    each debug file--|archs|, the full path to the top source
    directory--|srcdir|, for generating relative source file names,
    and an option to copy debug info files alongside the dumped
    symbol files--|copy_debug|, mostly useful for creating a
    Microsoft Symbol Server from the resulting output.

    You don't want to use this directly if you intend to call
    ProcessDir.  Instead, call GetPlatformSpecificDumper to
    get an instance of a subclass."""
    def __init__(self, dump_syms, symbol_path,
                 archs=None, srcdir=None, copy_debug=False, vcsinfo=False):
        
        self.dump_syms = os.path.abspath(dump_syms)
        self.symbol_path = symbol_path
        if archs is None:
            
            self.archs = ['']
        else:
            self.archs = ['-a %s' % a for a in archs.split()]
        if srcdir is not None:
            self.srcdir = os.path.normpath(srcdir)
        else:
            self.srcdir = None
        self.copy_debug = copy_debug
        self.vcsinfo = vcsinfo

    
    def ShouldProcess(self, file):
        return False

    def RunFileCommand(self, file):
        """Utility function, returns the output of file(1)"""
        try:
            
            
            return os.popen("file -Lb " + file).read()
        except:
            return ""

    
    def FixFilenameCase(self, file):
        return file

    def Process(self, file_or_dir):
        "Process a file or all the (valid) files in a directory."
        if os.path.isdir(file_or_dir):
            return self.ProcessDir(file_or_dir)
        elif os.path.isfile(file_or_dir):
            return self.ProcessFile(file_or_dir)
        
        return False
    
    def ProcessDir(self, dir):
        """Process all the valid files in this directory.  Valid files
        are determined by calling ShouldProcess."""
        result = True
        for root, dirs, files in os.walk(dir):
            for f in files:
                fullpath = os.path.join(root, f)
                if self.ShouldProcess(fullpath):
                    if not self.ProcessFile(fullpath):
                        result = False
        return result
    
    def ProcessFile(self, file):
        """Dump symbols from this file into a symbol file, stored
        in the proper directory structure in  |symbol_path|."""
        result = False
        for arch in self.archs:
            try:
                cmd = os.popen("%s %s %s" % (self.dump_syms, arch, file), "r")
                module_line = cmd.next()
                if module_line.startswith("MODULE"):
                    
                    (guid, debug_file) = (module_line.split())[3:5]
                    
                    sym_file = re.sub("\.pdb$", "", debug_file) + ".sym"
                    
                    rel_path = os.path.join(debug_file,
                                            guid,
                                            sym_file).replace("\\", "/")
                    full_path = os.path.normpath(os.path.join(self.symbol_path,
                                                              rel_path))
                    try:
                        os.makedirs(os.path.dirname(full_path))
                    except OSError: 
                        pass
                    f = open(full_path, "w")
                    f.write(module_line)
                    
                    for line in cmd:
                        if line.startswith("FILE"):
                            
                            (x, index, filename) = line.split(None, 2)
                            filename = self.FixFilenameCase(filename.rstrip())
                            if self.vcsinfo:
                                filename = GetVCSFilename(filename, self.srcdir)
                            f.write("FILE %s %s\n" % (index, filename))
                        else:
                            
                            f.write(line)
                    f.close()
                    cmd.close()
                    
                    
                    print rel_path
                    if self.copy_debug:
                        rel_path = os.path.join(debug_file,
                                                guid,
                                                debug_file).replace("\\", "/")
                        print rel_path
                        full_path = os.path.normpath(os.path.join(self.symbol_path,
                                                                  rel_path))
                        shutil.copyfile(file, full_path)
                    result = True
            except StopIteration:
                pass
            except:
                print >> sys.stderr, "Unexpected error: ", sys.exc_info()[0]
                raise
        return result




class Dumper_Win32(Dumper):
    def ShouldProcess(self, file):
        """This function will allow processing of pdb files that have dll
        or exe files with the same base name next to them."""
        if file.endswith(".pdb"):
            (path,ext) = os.path.splitext(file)
            if os.path.isfile(path + ".exe") or os.path.isfile(path + ".dll"):
                return True
        return False
    
    def FixFilenameCase(self, file):
        """Recent versions of Visual C++ put filenames into
        PDB files as all lowercase.  If the file exists
        on the local filesystem, fix it."""
        (path, filename) = os.path.split(file)
        if not os.path.isdir(path):
            return file
        lc_filename = filename.lower()
        for f in os.listdir(path):
            if f.lower() == lc_filename:
                return os.path.join(path, f)
        return file

class Dumper_Linux(Dumper):
    def ShouldProcess(self, file):
        """This function will allow processing of files that are
        executable, or end with the .so extension, and additionally
        file(1) reports as being ELF files.  It expects to find the file
        command in PATH."""
        if file.endswith(".so") or os.access(file, os.X_OK):
            return self.RunFileCommand(file).startswith("ELF")
        return False

class Dumper_Mac(Dumper):
    def ShouldProcess(self, file):
        """This function will allow processing of files that are
        executable, or end with the .dylib extension, and additionally
        file(1) reports as being Mach-O files.  It expects to find the file
        command in PATH."""
        if file.endswith(".dylib") or os.access(file, os.X_OK):
            return self.RunFileCommand(file).startswith("Mach-O")
        return False


def main():
    parser = OptionParser(usage="usage: %prog [options] <dump_syms binary> <symbol store path> <debug info files>")
    parser.add_option("-c", "--copy",
                      action="store_true", dest="copy_debug", default=False,
                      help="Copy debug info files into the same directory structure as symbol files")
    parser.add_option("-a", "--archs",
                      action="store", dest="archs",
                      help="Run dump_syms -a <arch> for each space separated cpu architecture in ARCHS (only on OS X)")
    parser.add_option("-s", "--srcdir",
                      action="store", dest="srcdir",
                      help="Use SRCDIR to determine relative paths to source files")
    parser.add_option("-v", "--vcs-info",
                      action="store_true", dest="vcsinfo",
                      help="Try to retrieve VCS info for each FILE listed in the output")
    (options, args) = parser.parse_args()

    if len(args) < 3:
        parser.error("not enough arguments")
        exit(1)

    dumper = GetPlatformSpecificDumper(dump_syms=args[0],
                                       symbol_path=args[1],
                                       copy_debug=options.copy_debug,
                                       archs=options.archs,
                                       srcdir=options.srcdir,
                                       vcsinfo=options.vcsinfo)
    for arg in args[2:]:
        dumper.Process(arg)


if __name__ == "__main__":
    main()
