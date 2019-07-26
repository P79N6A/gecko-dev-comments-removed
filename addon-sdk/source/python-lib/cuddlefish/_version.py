










git_refnames = "$Format:%d$"
git_full = "$Format:%H$"


import subprocess

def run_command(args, cwd=None, verbose=False):
    try:
        
        p = subprocess.Popen(args, stdout=subprocess.PIPE, cwd=cwd)
    except EnvironmentError, e:
        if verbose:
            print "unable to run %s" % args[0]
            print e
        return None
    stdout = p.communicate()[0].strip()
    if p.returncode != 0:
        if verbose:
            print "unable to run %s (error)" % args[0]
        return None
    return stdout


import sys
import re
import os.path

def get_expanded_variables(versionfile_source):
    
    
    
    
    variables = {}
    try:
        for line in open(versionfile_source,"r").readlines():
            if line.strip().startswith("git_refnames ="):
                mo = re.search(r'=\s*"(.*)"', line)
                if mo:
                    variables["refnames"] = mo.group(1)
            if line.strip().startswith("git_full ="):
                mo = re.search(r'=\s*"(.*)"', line)
                if mo:
                    variables["full"] = mo.group(1)
    except EnvironmentError:
        pass
    return variables

def versions_from_expanded_variables(variables, tag_prefix):
    refnames = variables["refnames"].strip()
    if refnames.startswith("$Format"):
        return {} 
    refs = set([r.strip() for r in refnames.strip("()").split(",")])
    for ref in list(refs):
        if not re.search(r'\d', ref):
            refs.discard(ref)
            
            
            
            
            
            
    for ref in sorted(refs):
        
        if ref.startswith(tag_prefix):
            r = ref[len(tag_prefix):]
            return { "version": r,
                     "full": variables["full"].strip() }
    
    return { "version": variables["full"].strip(),
             "full": variables["full"].strip() }

def versions_from_vcs(tag_prefix, versionfile_source, verbose=False):
    
    
    
    
    
    
    
    

    try:
        here = os.path.abspath(__file__)
    except NameError:
        
        return {} 

    
    
    
    root = here
    for i in range(len(versionfile_source.split("/"))):
        root = os.path.dirname(root)
    if not os.path.exists(os.path.join(root, ".git")):
        return {}

    GIT = "git"
    if sys.platform == "win32":
        GIT = "git.cmd"
    stdout = run_command([GIT, "describe", "--tags", "--dirty", "--always"],
                         cwd=root)
    if stdout is None:
        return {}
    if not stdout.startswith(tag_prefix):
        if verbose:
            print "tag '%s' doesn't start with prefix '%s'" % (stdout, tag_prefix)
        return {}
    tag = stdout[len(tag_prefix):]
    stdout = run_command([GIT, "rev-parse", "HEAD"], cwd=root)
    if stdout is None:
        return {}
    full = stdout.strip()
    if tag.endswith("-dirty"):
        full += "-dirty"
    return {"version": tag, "full": full}


def versions_from_parentdir(parentdir_prefix, versionfile_source, verbose=False):
    try:
        here = os.path.abspath(__file__)
        
        
        
        
        root = here
        for i in range(len(versionfile_source.split("/"))):
            root = os.path.dirname(root)
    except NameError:
        
        
        
        
        
        
        
        root = os.path.dirname(os.path.abspath(sys.argv[0]))
    
    
    dirname = os.path.basename(root)
    if not dirname.startswith(parentdir_prefix):
        if verbose:
            print "dirname '%s' doesn't start with prefix '%s'" %                   (dirname, parentdir_prefix)
        return None
    return {"version": dirname[len(parentdir_prefix):], "full": ""}

tag_prefix = ""
parentdir_prefix = "addon-sdk-"
versionfile_source = "python-lib/cuddlefish/_version.py"

def get_versions():
    variables = { "refnames": git_refnames, "full": git_full }
    ver = versions_from_expanded_variables(variables, tag_prefix)
    if not ver:
        ver = versions_from_vcs(tag_prefix, versionfile_source)
    if not ver:
        ver = versions_from_parentdir(parentdir_prefix, versionfile_source)
    if not ver:
        ver = {"version": "unknown", "full": ""}
    return ver

