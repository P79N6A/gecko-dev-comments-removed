





































import sys
import re
import string

defines = dict()

def emitShader(fp, shadername, shaderlines):
    eolContinue = "\\n\\\n";
    fp.write("const char %s[] = \"/* %s */%s" % (shadername,shadername,eolContinue))
    for line in shaderlines:
        line.replace("\\", "\\\\")
        while line.find('$') != -1:
            expansions = re.findall('\$\S+\$', line)
            for m in expansions:
                mkey = m[1:-1]
                if not defines.has_key(mkey):
                    print "Error: Undefined expansion used: '%s'" % (m,)
                    sys.exit(1)
                mval = defines[mkey]
                if type(mval) == str:
                    line = line.replace(m, mval)
                elif type(mval) == list:
                    line = line.replace(m, eolContinue.join(mval) + eolContinue);
                else:
                    print "Internal Error: Unknown type in defines array: '%s'" % (str(type(mval)),)

        fp.write("%s%s" % (line,eolContinue))
    fp.write("\";\n\n");

def genShaders(infile, outfile):
    source = open(infile, "r").readlines()
    desthdr = open(outfile, "w+")

    desthdr.write("/* AUTOMATICALLY GENERATED */\n");
    desthdr.write("/* DO NOT EDIT! */\n\n");

    global defines

    indefine = None
    inshader = None

    inblock = False
    linebuffer = []

    for line in source:
        
        if not inblock and line.startswith("//"):
            continue
        line = string.strip(line)

        if len(line) == 0:
            continue

        if line[0] == '@':
            cmd = line
            rest = ''

            if line.find(' ') != -1:
                cmd = line[0:line.find(' ')]
                rest = string.strip(line[len(cmd) + 1:])
                
            if cmd == "@define":
                if inblock:
                    raise Exception("@define inside a block!")
                space = rest.find(' ')
                if space != -1:
                    defines[rest[0:space]] = rest[space+1:]
                else:
                    indefine = rest
                    inblock = True
            elif cmd == "@shader":
                if inblock:
                    raise Exception("@shader inside a block!")
                if len(rest) == 0:
                    raise Exception("@shader without a name!")
                inshader = rest
                inblock = True
            elif cmd == "@end":
                if indefine is not None:
                    if type(linebuffer) == list:
                        for i in range(len(linebuffer)):
                            linebuffer[i] = linebuffer[i].replace("\\", "\\\\")
                    defines[indefine] = linebuffer
                elif inshader is not None:
                    emitShader(desthdr, inshader, linebuffer)
                else:
                    raise Exception("@end outside of a block!")
                indefine = None
                inshader = None
                inblock = None
                linebuffer = []
            else:
                raise Exception("Unknown command: %s" % (cmd,))
        else:
            if inblock:
                linebuffer.append(line)

if (len(sys.argv) != 3):
    print "Usage: %s infile.txt outfile.h" % (sys.argv[0],)
    sys.exit(1)

genShaders(sys.argv[1], sys.argv[2])
