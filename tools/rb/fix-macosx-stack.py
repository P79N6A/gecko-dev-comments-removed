
















































import subprocess
import sys
import re
import os.path

def separate_debug_file_for(file):
    return None

address_adjustments = {}
def address_adjustment(file):
    if not file in address_adjustments:
        result = None
        otool = subprocess.Popen(["otool", "-l", file], stdout=subprocess.PIPE)
        while True:
            line = otool.stdout.readline()
            if line == "":
                break
            if line == "  segname __TEXT\n":
                line = otool.stdout.readline()
                if not line.startswith("   vmaddr "):
                    raise StandardError("unexpected otool output")
                result = int(line[10:], 16)
                break
        otool.stdout.close()

        if result is None:
            raise StandardError("unexpected otool output")

        address_adjustments[file] = result

    return address_adjustments[file]



atoses = {}
def atos_proc(file):
    pipe = None
    if not file in atoses:
        debug_file = separate_debug_file_for(file) or file
        pipe = subprocess.Popen(['/usr/bin/atos', '-o', debug_file],
                                stdin=subprocess.PIPE,
                                stdout=subprocess.PIPE)
        
        
    else:
        pipe = atoses[file]
    return pipe

cxxfilt_proc = None
def cxxfilt(sym):
    if cxxfilt_proc is None:
        globals()["cxxfilt_proc"] = subprocess.Popen(['c++filt',
                                                      '--no-strip-underscores',
                                                      '--format', 'gnu-v3'],
                                                     stdin=subprocess.PIPE,
                                                     stdout=subprocess.PIPE)
    
    
    cxxfilt_proc.stdin.write(sym[1:] + "\n")
    return cxxfilt_proc.stdout.readline().rstrip("\n")

line_re = re.compile("^([ \|0-9-]*)(.*) ?\[([^ ]*) \+(0x[0-9A-F]{1,8})\](.*)$")
atos_sym_re = re.compile("^(\S+) \(in ([^)]+)\) \((.+)\)$")
for line in sys.stdin:
    result = line_re.match(line)
    if result is not None:
        
        
        (before, badsymbol, file, address, after) = result.groups()
        address = int(address, 16)

        if os.path.exists(file) and os.path.isfile(file):
            atos = atos_proc(file)
            address += address_adjustment(file)

            atos.stdin.write("0x%X\n" % address)
            atos.stdin.flush()
            
            
            atos.stdin.close()
            info = atos.stdout.readline().rstrip("\n")

            
            
            
            
            symresult = atos_sym_re.match(info)
            if symresult is not None:
                
                (symbol, library, fileline) = symresult.groups()
                symbol = cxxfilt(symbol)
                info = "%s (%s, in %s)" % (symbol, fileline, library)

            sys.stdout.write(before + info + after + "\n")
        else:
            sys.stderr.write("Warning: File \"" + file + "\" does not exist.\n")
            sys.stdout.write(line)
    else:
        sys.stdout.write(line)
