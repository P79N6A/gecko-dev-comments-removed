










from __future__ import print_function
import os
import re
import sys
import json


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
                "Android": "b2g" if env["MOZ_WIDGET_TOOLKIT"] == "gonk" else "android"}
    if o in known_os:
        d["os"] = known_os[o]
    else:
        
        d["os"] = o.lower()

    
    d["toolkit"] = env["MOZ_WIDGET_TOOLKIT"]
    
    
    if 'MOZ_APP_NAME' in env:
      d["appname"] = env["MOZ_APP_NAME"]

    
    p = env["TARGET_CPU"]
    
    if d["os"] == "mac" and "UNIVERSAL_BINARY" in env and env["UNIVERSAL_BINARY"] == "1":
        p = "universal-x86-x86_64"
    else:
        
        
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

def write_json(file, env=os.environ):
    """
    Write JSON data about the configuration specified in |env|
    to |file|, which may be a filename or file-like object.
    See build_dict for information about what  environment variables are used,
    and what keys are produced.
    """
    build_conf = build_dict(env)
    if isinstance(file, basestring):
        with open(file, "w") as f:
            json.dump(build_conf, f)
    else:
        json.dump(build_conf, file)


if __name__ == '__main__':
    try:
        write_json(sys.argv[1] if len(sys.argv) > 1 else sys.stdout)
    except Exception as e:
        print(str(e), file=sys.stderr)
        sys.exit(1)
