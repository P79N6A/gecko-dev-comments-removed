



import os, os.path
import subprocess
import sys

CL_INCLUDES_PREFIX = os.environ.get("CL_INCLUDES_PREFIX", "Note: including file:")

def InvokeClWithDependencyGeneration(cmdline):
    target = ""
    
    for arg in cmdline:
        if arg.startswith("-Fo"):
            target = arg[3:]
            break

    if target == None:
        print >>sys.stderr, "No target set" and sys.exit(1)

    
    depstarget = os.path.basename(target) + ".pp"

    cmdline += ['-showIncludes']
    cl = subprocess.Popen(cmdline, stdout=subprocess.PIPE)

    deps = set()
    for line in cl.stdout:
        
        
        if line.startswith(CL_INCLUDES_PREFIX):
            dep = line[len(CL_INCLUDES_PREFIX):].strip()
            
            
            
            if dep.find(' ') == -1:
                deps.add(dep)
        else:
            sys.stdout.write(line) 
                                   

    ret = cl.wait()
    if ret != 0 or target == "":
        sys.exit(ret)

    depsdir = os.path.normpath(os.path.join(os.curdir, ".deps"))
    depstarget = os.path.join(depsdir, depstarget)
    if not os.path.isdir(depsdir):
        try:
            os.makedirs(depsdir)
        except OSError:
            pass 
                 
                 

    f = open(depstarget, "w")
    for dep in sorted(deps):
        print >>f, "%s: %s" % (target, dep)

if __name__ == "__main__":
    InvokeClWithDependencyGeneration(sys.argv[1:])
