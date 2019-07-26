



import os, sys
import base64
import simplejson as json

def create_jid():
    """Return 'jid1-XYZ', where 'XYZ' is a randomly-generated string. (in the
    previous jid0- series, the string securely identified a specific public
    key). To get a suitable add-on ID, append '@jetpack' to this string.
    """
    
    
    
    
    
    
    
    
    

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    h = os.urandom(80/8)
    s = base64.b64encode(h, "AB").strip("=")
    jid = "jid1-" + s
    return jid

def preflight_config(target_cfg, filename, stderr=sys.stderr):
    modified = False
    config = json.load(open(filename, 'r'))

    if "id" not in config:
        print >>stderr, ("No 'id' in package.json: creating a new ID for you.")
        jid = create_jid()
        config["id"] = jid
        modified = True

    if modified:
        i = 0
        backup = filename + ".backup"
        while os.path.exists(backup):
            if i > 1000:
                raise ValueError("I'm having problems finding a good name"
                                 " for the backup file. Please move %s out"
                                 " of the way and try again."
                                 % (filename + ".backup"))
            backup = filename + ".backup-%d" % i
            i += 1
        os.rename(filename, backup)
        new_json = json.dumps(config, indent=4)
        open(filename, 'w').write(new_json+"\n")
        return False, True

    return True, False
