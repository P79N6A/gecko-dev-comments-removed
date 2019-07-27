






import os
import re
import json
import mozbuild.mozconfig as mozconfig

def build_dict(config, env=os.environ):
    """
    Build a dict containing data about the build configuration from
    the environment.
    """
    substs = config.substs

    
    required = ["TARGET_CPU", "OS_TARGET", "MOZ_WIDGET_TOOLKIT"]
    missing = [r for r in required if r not in substs]
    if missing:
        raise Exception("Missing required environment variables: %s" %
                        ', '.join(missing))

    d = {}
    d['topsrcdir'] = config.topsrcdir

    the_mozconfig = mozconfig.MozconfigLoader(config.topsrcdir).find_mozconfig(env)
    if the_mozconfig:
        d['mozconfig'] = the_mozconfig

    
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

    
    if 'MOZ_MULET' in substs and substs.get('MOZ_MULET') == "1":
        d["buildapp"] = "mulet"
    elif 'MOZ_BUILD_APP' in substs:
        d["buildapp"] = substs["MOZ_BUILD_APP"]

    
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
    d['crashreporter'] = bool(substs.get('MOZ_CRASHREPORTER'))
    d['datareporting'] = bool(substs.get('MOZ_DATA_REPORTING'))
    d['healthreport'] = substs.get('MOZ_SERVICES_HEALTHREPORT') == '1'
    d['asan'] = substs.get('MOZ_ASAN') == '1'
    d['tests_enabled'] = substs.get('ENABLE_TESTS') == "1"
    d['bin_suffix'] = substs.get('BIN_SUFFIX', '')

    d['webm'] = bool(substs.get('MOZ_WEBM'))
    d['wave'] = bool(substs.get('MOZ_WAVE'))

    return d


def write_mozinfo(file, config, env=os.environ):
    """Write JSON data about the configuration specified in config and an
    environment variable dict to |file|, which may be a filename or file-like
    object.
    See build_dict for information about what  environment variables are used,
    and what keys are produced.
    """
    build_conf = build_dict(config, env)
    if isinstance(file, basestring):
        with open(file, "w") as f:
            json.dump(build_conf, f)
    else:
        json.dump(build_conf, file)
