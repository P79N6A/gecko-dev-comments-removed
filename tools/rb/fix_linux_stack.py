


















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
        return (self.r.readline().rstrip("\r\n"), self.r.readline().rstrip("\r\n"))
    @staticmethod
    def test():
        assert unbufferedLineConverter("rev").convert("123") == "321"
        assert unbufferedLineConverter("cut", ["-c3"]).convert("abcde") == "c"
        print "Pass"

def separate_debug_file_for(file):
    return None

addr2lines = {}
def addressToSymbol(file, address):
    converter = None
    if not file in addr2lines:
        debug_file = separate_debug_file_for(file) or file
        converter = unbufferedLineConverter('/usr/bin/addr2line', ['-C', '-f', '-e', debug_file])
        addr2lines[file] = converter
    else:
        converter = addr2lines[file]
    return converter.convert(address)

line_re = re.compile("^(.*) ?\[([^ ]*) \+(0x[0-9a-f]{1,8})\](.*)$")
balance_tree_re = re.compile("^([ \|0-9-]*)")

def fixSymbols(line):
    result = line_re.match(line)
    if result is not None:
        
        
        (before, file, address, after) = result.groups()
        

        if os.path.exists(file) and os.path.isfile(file):
            
            (name, fileline) = addressToSymbol(file, address)
            info = "%s (%s)" % (name, fileline)

            
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
