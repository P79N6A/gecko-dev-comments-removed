














































































import sys
import os
from os.path import join
from subprocess import call, STDOUT
import fnmatch
import string
import shutil

CompressFlag = "/compress"
FaststartFlag = 0

class FileEntry:
    def __init__(self, dirpath, dircount, filename, filecount, actual_filename):
        self.dirpath = dirpath
        self.dircount = dircount
        self.filename = filename
        self.filecount = filecount
        self.actual_filename = actual_filename


class DirEntry:
    def __init__(self, dirpath, dircount, filecount):
        self.dirpath = dirpath
        self.dircount = dircount
        self.filecount = filecount


ignored_patterns = ['*~', '.#*', '#*#', '*.orig', '*.rej']

file_entry_count = 0
file_entries = []

fcount = 0
fnames = {}

directories = {}
files_in_directory = []





def read_exceptions(filename):
    exceptions = set()
    if os.path.exists(filename):
        f = file(filename)
        for line in f:
            line = line.strip()
            if line and line[0] != '#':
                exceptions.add(line)
        exceptions.add( os.path.basename(filename) )
        f.close()
    return exceptions






def sort_dircount_then_filename(item1, item2):
    
    if item1.dircount != item2.dircount:
        return cmp(item1.dircount, item2.dircount)

    
    next_result = cmp(item1.filename, item2.filename)
    if next_result != 0:
        return next_result

    
    return cmp(item1.filecount, item2.filecount)

    
def handle_duplicate_filename(dirpath, filename, filecount):
    if filecount == 1:
        return filename
    file_parts = os.path.splitext(filename)
    new_filename = "%s_%d%s" % (file_parts[0], filecount, file_parts[1])
    old_relative_filename = join(dirpath, filename)
    new_relative_filename = join(dirpath, new_filename)
    shutil.copyfile(old_relative_filename, new_relative_filename)
    return new_filename

def add_new_entry(dirpath, dircount, filename, filecount):
    global file_entries
    global file_entry_count
    actual_filename = handle_duplicate_filename(dirpath, filename, filecount)
    file_entries.append( FileEntry(dirpath, dircount, filename, filecount, actual_filename) )
    file_entry_count += 1


def walk_tree(start_dir, ignore):
    global fcount
    global fnames
    global directories
    global files_in_directory
    
    dircount = 0;
    files_in_directory = [0]

    for (dirpath, dirnames, filenames) in os.walk(start_dir):
        exceptions = read_exceptions(join(dirpath, 'install-exceptions'))

        dircount += 1
        directories[dirpath] = DirEntry(dirpath, dircount, len(filenames))

        if len(filenames) < 1:
            print "WARNING: No files in directory [%s]" % dirpath

        for filename in filenames:
            if len(exceptions) > 0 and filename in exceptions:
                continue
            if any(fnmatch.fnmatch(filename, p) for p in ignore):
                continue
            filecount = 1
            if filename in fnames:
                filecount = fnames[filename] + 1
            fnames[filename] = filecount

            add_new_entry(dirpath, dircount, filename, filecount)


def output_inf_file_header(f, program_name):
    f.write("""; Duplicated Filenames ==> One of the two files
;   needs renaming before packaging up the CAB
;
; Technique: Rename the second directory's file to XXXX_1 prior to starting the CABWIZ,
;   then take care of the name in the File.xxxxx section

[Version]
Signature   = "$Windows NT$"        ; required as-is
Provider    = "Mozilla"             ; maximum of 30 characters, full app name will be \"<Provider> <AppName>\"
CESignature = "$Windows CE$"        ; required as-is

[CEStrings]
AppName     = "%s"              ; maximum of 40 characters, full app name will be \"<Provider> <AppName>\"\n""" % program_name)

    f.write("InstallDir  = %CE1%\\%AppName%       ; Program Files\Fennec\n\n")


def output_sourcedisksnames_section(f, dirs):
    f.write("[SourceDisksNames]                  ; directory that holds the application's files\n")
    for dir in dirs:
        f.write("""%d = , "%s",,%s\n""" % (directories[dir].dircount, dir, dir))
    f.write(" \n")


def output_sourcedisksfiles_section(f):
    f.write("[SourceDisksFiles]                  ; list of files to be included in .cab\n")
    for entry in sorted(file_entries, sort_dircount_then_filename):
        f.write("%s=%d\n" % (entry.actual_filename, entry.dircount))
    f.write("\n")


def output_defaultinstall_section(f, dirs):
    copyfileslist = ""
    copyfilesrawlist=[]
    for dir in dirs:
        if directories[dir].filecount < 1:
            continue
        copyfilesrawlist.append( "Files.%s" % '.'.join( dir.split('\\') ) )
        prefix = ", "
    copyfileslist = ','.join(copyfilesrawlist)

    f.write("""[DefaultInstall]                    ; operations to be completed during install
CopyFiles   = %s
AddReg      = RegData
CEShortcuts = Shortcuts
\n""" % copyfileslist)


def output_destinationdirs_section(f, dirs):
    f.write("[DestinationDirs]                   ; default destination directories for each operation section\n")
    for dir in dirs:
        dir_split = dir.split('\\')
        mod_dir_string = '.'.join(dir_split)
        if len(dir_split) > 1:
            dir_minus_top_level = '\\'.join(dir_split[1:])
        else:
            dir_minus_top_level = ""
        if dir_minus_top_level:
            dir_minus_top_level = "\\%s" % dir_minus_top_level
        if directories[dir].filecount < 1:
            f.write(";Files.%s = 0, %%InstallDir%%%s          ; NO FILES IN THIS DIRECTORY\n" % (mod_dir_string, dir_minus_top_level))
        else:
            f.write("Files.%s = 0, %%InstallDir%%%s\n" % (mod_dir_string, dir_minus_top_level))
    f.write("Shortcuts   = 0, %CE11%             ; \Windows\Start Menu\Programs\n\n")


def output_directory_sections(f, dirs):
    for dir in dirs:
        if directories[dir].filecount < 1:
            f.write(";[Files.%s]\n;===NO FILES===\n" % '.'.join( dir.split('\\') ))
        else:
            f.write("[Files.%s]\n" % '.'.join( dir.split('\\') ))
            for entry in file_entries:
                if entry.dirpath == dir:
                    f.write("\"%s\",%s\n" % (entry.filename, entry.actual_filename))
        f.write("\n")


def output_registry_section(f):
    f.write("""[RegData]                           ; registry key list
HKCU,Software\%AppName%,MajorVersion,0x00010001,1
HKCU,Software\%AppName%,MinorVersion,0x00010001,0
\n""")


def output_shortcuts_section(f, app_name):
    f.write("[Shortcuts]\n")
    f.write("%%AppName%%,0,%s,%%CE11%%\n" % app_name)
    if FaststartFlag:
        f.write("%%AppName%%faststart,0,%sfaststart.exe,%%CE4%%\n" % os.path.splitext(app_name)[0]);


def output_inf_file(program_name, app_name):
    global files_in_directory
    inf_name = "%s.inf" % program_name
    f = open(inf_name, 'w')
    output_inf_file_header(f, program_name)
    dirs = sorted(directories)
    output_sourcedisksnames_section(f, dirs)
    output_sourcedisksfiles_section(f)
    output_defaultinstall_section(f, dirs)
    output_destinationdirs_section(f, dirs)
    output_directory_sections(f, dirs)
    output_registry_section(f)
    output_shortcuts_section(f, app_name)
    f.close()



def make_cab_file(cabwiz_path, program_name, cab_final_name):
    make_cab_command = "\"%s\" %s %s.inf" % (cabwiz_path, CompressFlag, program_name)
    print "INFORMATION: Executing command to make %s CAB file (only works on BASH)" % program_name
    print "    [%s]" % make_cab_command
    sys.stdout.flush()

    success = call([cabwiz_path, "%s.inf" % program_name, CompressFlag],
                   stdout=open("NUL:","w"), stderr=STDOUT)

    if not os.path.isfile("%s.CAB" % program_name):
        print """***************************************************************************
ERROR: CAB FILE NOT CREATED.
       You can try running the command by hand:
          %s" % make_cab_comman
 ---- 
 NOTE: If you see an error like this:
       Error: File XXXXXXXXXX.inf contains DirIDs, which are not supported
 -- 
 this may mean that your PYTHON is outputting Windows files WITHOUT CR-LF
 line endings.  Please verify that your INF file has CR-LF line endings.
***************************************************************************"""
        sys.exit(2)

    print "INFORMATION: Executing command to move %s.CAB to %s" % (program_name, cab_final_name)
    sys.stdout.flush()
    shutil.move("%s.CAB" % program_name, cab_final_name)


def purge_copied_files():
    for entry in file_entries:
        if entry.filename != entry.actual_filename:
            new_relative_filename = join(entry.dirpath, entry.actual_filename)
            os.remove(new_relative_filename)


def main():
    args = sys.argv
    if len(args) < 4 or len(args) > 7:
        print >> sys.stderr, "Usage: %s [-s] [-faststart] [CABWIZ_PATH] SOURCE_DIR PROGRAM_NAME CAB_FINAL_NAME" % args[0]
        print >> sys.stderr, "Example: %s /c/Program\ Files/Microsoft\ Visual\ Studio\ 9.0/ fennec Fennec fennec-0.11.en-US.wince-arm.cab" % args[0]
        sys.exit(1)

    args = args[1:]

    if args[0] == "-s":
        global CompressFlag
        CompressFlag = ""
        args = args[1:]

    if args[0] == "-faststart":
        global FaststartFlag
        FaststartFlag = 1
        args = args[1:]

    cabwiz_path = None
    if len(args) == 4:
        cabwiz_path = args[0]
        args = args[1:]
    else:
        if "VSINSTALLDIR" in os.environ:
            cabwiz_path = os.path.join(os.environ["VSINSTALLDIR"], "SmartDevices", "SDK", "SDKTools", "cabwiz.exe")

    source_dir = args[0]
    program_name = args[1]
    app_name = "%s.exe" % source_dir
    cab_final_name = args[2]

    if cabwiz_path is None or not os.path.isfile(cabwiz_path):
        print """***************************************************************************
ERROR: CABWIZ_PATH is not a valid file, or cabwiz couldn't be found!
       EXITING...
***************************************************************************"""
        sys.exit(2)

    walk_tree(source_dir, ignored_patterns)
    sys.stdout.flush()
    output_inf_file(program_name, app_name)
    sys.stdout.flush()
    make_cab_file(cabwiz_path, program_name, cab_final_name)
    purge_copied_files()



if __name__ == "__main__":
    main()
