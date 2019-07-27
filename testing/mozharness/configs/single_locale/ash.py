

















config = {
    "nightly_build": True,
    "branch": "ash",
    "en_us_binary_url": "http://ftp.mozilla.org/pub/mozilla.org/firefox/nightly/latest-mozilla-central/",
    "update_channel": "nightly",
    "latest_mar_dir": '/pub/mozilla.org/firefox/nightly/latest-mozilla-central-l10n',

    
    "hg_l10n_base": "https://hg.mozilla.org/l10n-central",

    
    "enable_partials": True,
    "mar_tools_url": "http://ftp.mozilla.org/pub/mozilla.org/firefox/nightly/latest-mozilla-central/mar-tools/%(platform)s",
    "previous_mar_url": "http://ftp.mozilla.org/pub/mozilla.org/firefox/nightly/latest-mozilla-central-l10n",

    
    "mozilla_dir": "ash",
    "repos": [{
        "vcs": "hg",
        "repo": "https://hg.mozilla.org/build/tools",
        "revision": "default",
        "dest": "tools",
    }, {
        "vcs": "hgtool",
        "repo": "https://hg.mozilla.org/mozilla-central",
        "revision": "default",
        "dest": "ash",
    }, {
        "vcs": "hgtool",
        "repo": "https://hg.mozilla.org/build/compare-locales",
        "revision": "RELEASE_AUTOMATION"
    }],
    
    'is_automation': True,
    'purge_minsize': 12,
}
