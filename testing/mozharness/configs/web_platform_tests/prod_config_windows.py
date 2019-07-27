







import os
import sys

config = {
    
    "in_tree_config": "config/mozharness/web_platform_tests_config.py",

    "exes": {
        'python': sys.executable,
        'virtualenv': [sys.executable, 'c:/mozilla-build/buildbotve/virtualenv.py'],
        'hg': 'c:/mozilla-build/hg/hg',
        'mozinstall': ['%s/build/venv/scripts/python' % os.getcwd(),
                       '%s/build/venv/scripts/mozinstall-script.py' % os.getcwd()],
        'tooltool.py': [sys.executable, 'C:/mozilla-build/tooltool.py'],
    },

    "options": [],

    "find_links": [
        "http://pypi.pvt.build.mozilla.org/pub",
        "http://pypi.pub.build.mozilla.org/pub",
    ],

    "pip_index": False,

    "buildbot_json_path": "buildprops.json",

    "default_blob_upload_servers": [
         "https://blobupload.elasticbeanstalk.com",
    ],

    "blob_uploader_auth_file" : os.path.join(os.getcwd(), "oauth.txt"),

    "download_minidump_stackwalk": True,
}
