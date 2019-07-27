

config = {
    
    "test_type": "browser",
    "marionette_address": "localhost:2828",
    "test_manifest": "unit-tests.ini",

    
    "installer_url": "http://stage.mozilla.org/pub/mozilla.org/firefox/tinderbox-builds/mozilla-central-linux-debug/1344372927/firefox-17.0a1.en-US.linux-i686.tar.bz2",
    "test_url": "http://stage.mozilla.org/pub/mozilla.org/firefox/tinderbox-builds/mozilla-central-linux-debug/1344372927/firefox-17.0a1.en-US.linux-i686.tests.zip",

    "default_actions": [
        'clobber',
        'download-and-extract',
        'create-virtualenv',
        'install',
        'run-marionette',
    ],
    "in_tree_config": "config/mozharness/marionette.py",
}
