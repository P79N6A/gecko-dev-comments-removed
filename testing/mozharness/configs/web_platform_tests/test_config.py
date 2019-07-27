





config = {
    
    "in_tree_config": "config/mozharness/web_platform_tests_config.py",

    "options": [],

    "default_actions": [
        'clobber',
        'download-and-extract',
        'create-virtualenv',
        'pull',
        'install',
        'run-tests',
    ],

    "find_links": [
        "http://pypi.pub.build.mozilla.org/pub",
    ],

    "pip_index": False,
}
