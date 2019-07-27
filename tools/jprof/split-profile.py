



















































import sys
import subprocess
import os.path

if len(sys.argv) < 5:
    sys.stderr.write("Expected arguments: <jprof> <split-file> <program> <jprof-log>\n")
    sys.exit(1)

jprof = sys.argv[1]
splitfile = sys.argv[2]
passthrough = sys.argv[3:]

for f in [jprof, splitfile]:
    if not os.path.isfile(f):
        sys.stderr.write("could not find file: {0}\n".format(f))
        sys.exit(1)

def read_splits(splitfile):
    """
    Read splitfile (each line of which contains a name, a space, and
    then a function name to split on), and return a list of pairs
    representing exactly that.  (Note that the name cannot contain
    spaces, but the function name can, and often does.)
    """
    def line_to_split(line):
        line = line.strip("\r\n")
        idx = line.index(" ")
        return (line[0:idx], line[idx+1:])

    io = open(splitfile, "r")
    result = [line_to_split(line) for line in io]
    io.close()
    return result

splits = read_splits(splitfile)

def generate_profile(options, destfile):
    """
    Run jprof to generate one split of the profile.
    """
    args = [jprof] + options + passthrough
    print "Generating {0}".format(destfile)
    destio = open(destfile, "w")
    
    cwd = None
    for option in passthrough:
        if option.find("jprof-log"):
            cwd = os.path.dirname(option)
    if cwd is None:
        raise StandardError("no jprof-log option given")
    process = subprocess.Popen(args, stdout=destio, cwd=cwd)
    process.wait()
    destio.close()
    if process.returncode != 0:
        os.remove(destfile)
        sys.stderr.write("Error {0} from command:\n  {1}\n".format(process.returncode, " ".join(args)))
        sys.exit(process.returncode)

def output_filename(number, splitname):
    """
    Return the filename (absolute path) we should use to output the
    profile segment with the given number and splitname.  Splitname
    should be None for the complete profile and the remainder.
    """
    def pad_count(i):
        result = str(i)
        
        result = "0" * (len(str(len(splits) + 1)) - len(result)) + result
        return result

    name = pad_count(number)
    if splitname is not None:
        name += "-" + splitname

    return os.path.join(os.path.dirname(splitfile),
                        "jprof-{0}.html".format(name))


generate_profile([], output_filename(0, None))


count = 1
excludes = []
for (splitname, splitfunction) in splits:
    generate_profile(excludes + ["-i" + splitfunction],
                     output_filename(count, splitname))
    excludes += ["-e" + splitfunction]
    count = count + 1


generate_profile(excludes, output_filename(count, None))
