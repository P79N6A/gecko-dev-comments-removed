






import os
import cStringIO
import WebIDL
import cPickle
from Configuration import *
from Codegen import GlobalGenRoots, replaceFileIfChanged

import Codegen

def generate_file(config, name, action):

    root = getattr(GlobalGenRoots, name)(config)
    if action is 'declare':
        filename = name + '.h'
        code = root.declare()
    else:
        assert action is 'define'
        filename = name + '.cpp'
        code = root.define()

    if replaceFileIfChanged(filename, code):
        print "Generating %s" % (filename)
    else:
        print "%s hasn't changed - not touching it" % (filename)

def main():
    
    from optparse import OptionParser
    usageString = "usage: %prog [options] webidldir [files]"
    o = OptionParser(usage=usageString)
    o.add_option("--cachedir", dest='cachedir', default=None,
                 help="Directory in which to cache lex/parse tables.")
    o.add_option("--verbose-errors", action='store_true', default=False,
                 help="When an error happens, display the Python traceback.")
    o.add_option("--use-jsop-accessors", action='store_true', default=False,
                 dest='useJSOPAccessors',
                 help="Use JSPropertyOps instead of JSNatives for getters and setters")
    (options, args) = o.parse_args()
    Codegen.generateNativeAccessors = not options.useJSOPAccessors

    if len(args) < 2:
        o.error(usageString)

    configFile = args[0]
    baseDir = args[1]
    fileList = args[2:]

    
    parser = WebIDL.Parser(options.cachedir)
    for filename in fileList:
        fullPath = os.path.normpath(os.path.join(baseDir, filename))
        f = open(fullPath, 'rb')
        lines = f.readlines()
        f.close()
        parser.parse(''.join(lines), fullPath)
    parserResults = parser.finish()

    
    resultsFile = open('ParserResults.pkl', 'wb')
    cPickle.dump(parserResults, resultsFile, -1)
    resultsFile.close()

    
    config = Configuration(configFile, parserResults)

    
    generate_file(config, 'PrototypeList', 'declare')

    
    generate_file(config, 'RegisterBindings', 'declare')
    generate_file(config, 'RegisterBindings', 'define')

    generate_file(config, 'UnionTypes', 'declare')
    generate_file(config, 'UnionConversions', 'declare')

if __name__ == '__main__':
    main()
