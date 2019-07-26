



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

    
    source = cmdline[-1]
    assert not source.startswith('-')

    
    depstarget = os.path.basename(target) + ".pp"

    cmdline += ['-showIncludes']
    cl = subprocess.Popen(cmdline, stdout=subprocess.PIPE)

    deps = set([os.path.normcase(source).replace(os.sep, '/')])
    for line in cl.stdout:
        
        
        if line.startswith(CL_INCLUDES_PREFIX):
            dep = line[len(CL_INCLUDES_PREFIX):].strip()
            
            
            
            if ' ' not in dep:
                deps.add(os.path.normcase(dep).replace(os.sep, '/'))
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
                 
                 

    with open(depstarget, "w") as f:
        f.write("%s: %s" % (target, source))
        for dep in sorted(deps):
            f.write(" \\\n%s" % dep)
        f.write('\n')
        for dep in sorted(deps):
            f.write("%s:\n" % dep)

if __name__ == "__main__":
    InvokeClWithDependencyGeneration(sys.argv[1:])
