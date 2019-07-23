
















































import subprocess
import sys
import re
import os
import pty
import termios

class unbufferedLineConverter:
    """
    Wrap a child process that responds to each line of input with one line of
    output.  Uses pty to trick the child into providing unbuffered output.
    """
    def __init__(self, command, args = []):
        pid, fd = pty.fork()
        if pid == 0:
            
            os.execvp(command, [command] + args)
        else:
            
            attr = termios.tcgetattr(fd)
            attr[3] = attr[3] & ~termios.ECHO
            termios.tcsetattr(fd, termios.TCSANOW, attr)
            
            self.r = os.fdopen(fd, "r", 1)
            self.w = os.fdopen(os.dup(fd), "w", 1)
    def convert(self, line):
        self.w.write(line + "\n")
        return self.r.readline().rstrip("\r\n")
    @staticmethod
    def test():
        assert unbufferedLineConverter("rev").convert("123") == "321"
        assert unbufferedLineConverter("cut", ["-c3"]).convert("abcde") == "c"
        print "Pass"

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
def addressToSymbol(file, address):
    converter = None
    if not file in atoses:
        debug_file = separate_debug_file_for(file) or file
        converter = unbufferedLineConverter('/usr/bin/atos', ['-o', debug_file])
        atoses[file] = converter
    else:
        converter = atoses[file]
    return converter.convert("0x%X" % address)

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

line_re = re.compile("^(.*) ?\[([^ ]*) \+(0x[0-9A-F]{1,8})\](.*)$")
balance_tree_re = re.compile("^([ \|0-9-]*)")
atos_sym_re = re.compile("^(\S+) \(in ([^)]+)\) \((.+)\)$")

def fixSymbols(line):
    result = line_re.match(line)
    if result is not None:
        
        
        (before, file, address, after) = result.groups()
        address = int(address, 16)

        if os.path.exists(file) and os.path.isfile(file):
            address += address_adjustment(file)
            info = addressToSymbol(file, address)

            
            
            
            
            symresult = atos_sym_re.match(info)
            if symresult is not None:
                
                (symbol, library, fileline) = symresult.groups()
                symbol = cxxfilt(symbol)
                info = "%s (%s, in %s)" % (symbol, fileline, library)

             
            before = balance_tree_re.match(before).groups()[0]

            return before + info + after + "\n"
        else:
            sys.stderr.write("Warning: File \"" + file + "\" does not exist.\n")
            return line
    else:
        return line

if __name__ == "__main__":
    for line in sys.stdin:
        sys.stdout.write(fixSymbols(line))
