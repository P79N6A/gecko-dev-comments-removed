



































import os
import subprocess
import urllib

abs_path = os.path.dirname(os.path.abspath(__file__))
root_path = os.path.dirname(abs_path)


externalModules = [
    {   
        "url": "http://hg.mozilla.org/mozilla-central/raw-file/default/testing/mochitest/tests/SimpleTest/EventUtils.js",
        "path": "mozmill/extension/resource/stdlib/EventUtils.js",
        "patch": "patches/eventUtils.patch"
    },
    {   
        "url": "http://hg.mozilla.org/mozilla-central/raw-file/default/netwerk/test/httpserver/httpd.js",
        "path": "mozmill/extension/resource/stdlib/httpd.js",
        "patch": "patches/httpd.patch"
    }
]



os.chdir(root_path)

for module in externalModules:
    
    print "Downloading %s..." % (module["url"])
    urllib.urlretrieve (module["url"], os.path.join(root_path, module["path"]))

    print "Patching %s..." % (module["patch"])
    subprocess.call(["git", "apply", module["patch"]])
