






from __future__ import with_statement
import os, re, sys

def build_dict(env=os.environ):
    """
    Build a dict containing data about the build configuration from
    the environment.
    """
    d = {}
    
    required = ["TARGET_CPU", "OS_TARGET", "MOZ_WIDGET_TOOLKIT"]
    missing = [r for r in required if r not in env]
    if missing:
        raise Exception("Missing required environment variables: %s" %
                        ', '.join(missing))
    
    o = env["OS_TARGET"]
    known_os = {"Linux": "linux",
                "WINNT": "win",
                "Darwin": "mac",
                "Android": "android"}
    if o in known_os:
        d["os"] = known_os[o]
    else:
        
        d["os"] = o.lower()

    
    d["toolkit"] = env["MOZ_WIDGET_TOOLKIT"]
    
    
    p = env["TARGET_CPU"]
    
    
    if p.startswith("arm"):
        p = "arm"
    elif re.match("i[3-9]86", p):
        p = "x86"
    d["processor"] = p
    
    if p in ["x86_64", "ppc64"]:
        d["bits"] = 64
    
    elif p in ["x86", "arm", "ppc"]:
        d["bits"] = 32
    

    
    d["debug"] = 'MOZ_DEBUG' in env and env['MOZ_DEBUG'] == '1'

    
    d["crashreporter"] = 'MOZ_CRASHREPORTER' in env and env['MOZ_CRASHREPORTER'] == '1'
    return d


class JsonValue:
    """
    A class to serialize Python values into JSON-compatible representations.
    """
    def __init__(self, v):
        if v is not None and not (isinstance(v,str) or isinstance(v,bool) or isinstance(v,int)):
            raise Exception("Unhandled data type: %s" % type(v))
        self.v = v
    def __repr__(self):
        if self.v is None:
            return "null"
        if isinstance(self.v,bool):
            return str(self.v).lower()
        return repr(self.v)

def jsonify(d):
    """
    Return a JSON string of the dict |d|. Only handles a subset of Python
    value types: bool, str, int, None.
    """
    jd = {}
    for k, v in d.iteritems():
        jd[k] = JsonValue(v)
    return repr(jd)

def write_json(file, env=os.environ):
    """
    Write JSON data about the configuration specified in |env|
    to |file|, which may be a filename or file-like object.
    See build_dict for information about what  environment variables are used,
    and what keys are produced.
    """
    s = jsonify(build_dict(env))
    if isinstance(file, basestring):
        with open(file, "w") as f:
            f.write(s)
    else:
        file.write(s)

if __name__ == '__main__':
    try:
        write_json(sys.argv[1] if len(sys.argv) > 1 else sys.stdout)
    except Exception, e:
        print >>sys.stderr, str(e)
        sys.exit(1)
