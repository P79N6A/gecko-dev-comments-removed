



import sys
import string

propList = eval(sys.stdin.read())
props = ""
for [prop, pref] in propList:
    pref = '[Pref=%s] ' % pref if pref is not "" else ""
    if not prop.startswith("Moz"):
        prop = prop[0].lower() + prop[1:]
    
    
    props += "  %sattribute DOMString %s;\n" % (pref, prop)

idlFile = open(sys.argv[1], "r");
idlTemplate = idlFile.read();
idlFile.close();

print string.Template(idlTemplate).substitute({ "props": props })
