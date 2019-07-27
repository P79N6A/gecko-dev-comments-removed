config = {
    'default_actions': [
        'clobber',
        'clone-tools',
        'checkout-sources',
        'setup-mock',
        'package-source',
    ],
    'stage_platform': 'source',  
    'purge_minsize': 3,
    'buildbot_json_path': 'buildprops.json',
    'app_ini_path': 'FAKE',  
    'objdir': 'obj-firefox',
    'env': {
        'MOZ_OBJDIR': 'obj-firefox',
        'TINDERBOX_OUTPUT': '1',
        'LC_ALL': 'C',
    },
}
