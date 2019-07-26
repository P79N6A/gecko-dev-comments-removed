



import os
import cPickle
from Configuration import Configuration
from Codegen import CGBindingRoot, replaceFileIfChanged

def generate_binding_header(config, outputprefix, webidlfile):
    """
    |config| Is the configuration object.
    |outputprefix| is a prefix to use for the header guards and filename.
    """

    filename = outputprefix + ".h"
    root = CGBindingRoot(config, outputprefix, webidlfile)
    if replaceFileIfChanged(filename, root.declare()):
        print "Generating binding header: %s" % (filename)

def generate_binding_cpp(config, outputprefix, webidlfile):
    """
    |config| Is the configuration object.
    |outputprefix| is a prefix to use for the header guards and filename.
    """

    filename = outputprefix + ".cpp"
    root = CGBindingRoot(config, outputprefix, webidlfile)
    if replaceFileIfChanged(filename, root.define()):
        print "Generating binding implementation: %s" % (filename)

def main():

    
    from optparse import OptionParser
    usagestring = "usage: %prog [header|cpp] configFile outputPrefix webIDLFile"
    o = OptionParser(usage=usagestring)
    o.add_option("--verbose-errors", action='store_true', default=False,
                 help="When an error happens, display the Python traceback.")
    (options, args) = o.parse_args()

    if len(args) != 4 or (args[0] != "header" and args[0] != "cpp"):
        o.error(usagestring)
    buildTarget = args[0]
    configFile = os.path.normpath(args[1])
    outputPrefix = args[2]
    webIDLFile = os.path.normpath(args[3])

    
    f = open('ParserResults.pkl', 'rb')
    parserData = cPickle.load(f)
    f.close()

    
    config = Configuration(configFile, parserData)

    
    if buildTarget == "header":
        generate_binding_header(config, outputPrefix, webIDLFile);
    elif buildTarget == "cpp":
        generate_binding_cpp(config, outputPrefix, webIDLFile);
    else:
        assert False 

if __name__ == '__main__':
    main()
