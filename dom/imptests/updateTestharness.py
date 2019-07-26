




from __future__ import unicode_literals

import subprocess

repo = "https://dvcs.w3.org/hg/resources"
dest = "resources-upstream"
files = ["testharness.js", "testharness.css", "idlharness.js", "WebIDLParser.js"]

subprocess.check_call(["hg", "clone", repo, dest])
for f in files:
    subprocess.check_call(["cp", "%s/%s" % (dest, f), f])
    subprocess.check_call(["hg", "add", f])
subprocess.check_call(["rm", "-rf", dest])

