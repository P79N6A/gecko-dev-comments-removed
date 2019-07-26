










from __future__ import print_function
import os
import re
import sys
import json

import buildconfig

def build_dict(env=None):
    """
    Build a dict containing data about the build configuration from
    the environment.
    """
    substs = env or buildconfig.substs
    env = env or os.environ

    
    required = ["TARGET_CPU", "OS_TARGET", "MOZ_WIDGET_TOOLKIT"]
    missing = [r for r in required if r not in substs]
    if missing:
        raise Exception("Missing required environment variables: %s" %
                        ', '.join(missing))

    d = {}
    d['topsrcdir'] = substs.get('TOPSRCDIR', buildconfig.topsrcdir)

    if 'MOZCONFIG' in env:
        mozconfig = env["MOZCONFIG"]
        if 'TOPSRCDIR' in env:
            mozconfig = os.path.join(env["TOPSRCDIR"], mozconfig)
        d['mozconfig'] = os.path.normpath(mozconfig)

    
    o = substs["OS_TARGET"]
    known_os = {"Linux": "linux",
                "WINNT": "win",
                "Darwin": "mac",
                "Android": "b2g" if substs["MOZ_WIDGET_TOOLKIT"] == "gonk" else "android"}
    if o in known_os:
        d["os"] = known_os[o]
    else:
        
        d["os"] = o.lower()

    
    d["toolkit"] = substs["MOZ_WIDGET_TOOLKIT"]

    
    if 'MOZ_APP_NAME' in substs:
        d["appname"] = substs["MOZ_APP_NAME"]

    
    p = substs["TARGET_CPU"]
    
    if d["os"] == "mac" and "UNIVERSAL_BINARY" in substs and substs["UNIVERSAL_BINARY"] == "1":
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
    

    d['debug'] = substs.get('MOZ_DEBUG') == '1'
    d['crashreporter'] = substs.get('MOZ_CRASHREPORTER') == '1'
    d['asan'] = substs.get('MOZ_ASAN') == '1'
    d['tests_enabled'] = substs.get('ENABLE_TESTS') == "1"
    d['bin_suffix'] = substs.get('BIN_SUFFIX', '')

    return d

def write_json(file, env=None):
    """
    Write JSON data about the configuration specified in |env|
    to |file|, which may be a filename or file-like object.
    See build_dict for information about what  environment variables are used,
    and what keys are produced.
    """
    build_conf = build_dict(env=env)
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
