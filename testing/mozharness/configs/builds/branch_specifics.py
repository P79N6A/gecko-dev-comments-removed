





























config = {
    
    "mozilla-central": {
        "repo_path": 'mozilla-central',
        "update_channel": "nightly",
        "graph_server_branch_name": "Firefox",
        'use_branch_in_symbols_extra_buildid': False,
    },
    'mozilla-release': {
        'repo_path': 'releases/mozilla-release',
        
        
        'update_channel': 'release',
        'branch_uses_per_checkin_strategy': True,
        'use_branch_in_symbols_extra_buildid': False,
    },
    'mozilla-beta': {
        'repo_path': 'releases/mozilla-beta',
        
        
        'update_channel': 'beta',
        'branch_uses_per_checkin_strategy': True,
        'use_branch_in_symbols_extra_buildid': False,
    },
    'mozilla-aurora': {
        'repo_path': 'releases/mozilla-aurora',
        'update_channel': 'aurora',
        'branch_uses_per_checkin_strategy': True,
        'use_branch_in_symbols_extra_buildid': False,
    },
    'mozilla-esr31': {
        'repo_path': 'releases/mozilla-esr31',
        'update_channel': 'nightly-esr31',
        'branch_uses_per_checkin_strategy': True,
        'use_branch_in_symbols_extra_buildid': False,
    },
    'mozilla-b2g32_v2_0': {
        'repo_path': 'releases/mozilla-b2g32_v2_0',
        'use_branch_in_symbols_extra_buildid': False,
        'update_channel': 'nightly-b2g32',
        'graph_server_branch_name': 'Mozilla-B2g32-v2.0',
    },
    'try': {
        'repo_path': 'try',
        'clone_by_revision': True,
        'clone_with_purge': True,
        'tinderbox_build_dir': '%(who)s-%(got_revision)s',
        'to_tinderbox_dated': False,
        'include_post_upload_builddir': True,
        'release_to_try_builds': True,
        'use_branch_in_symbols_extra_buildid': False,
        'stage_username': 'trybld',
        'stage_ssh_key': 'trybld_dsa',
        'branch_supports_uploadsymbols': False,
        'use_clobberer': False,
    },

    
    'b2g-inbound': {
        'repo_path': 'integration/b2g-inbound',
    },
    'fx-team': {
        'repo_path': 'integration/fx-team',
    },
    'gum': {
        'branch_uses_per_checkin_strategy': True,
    },
    'mozilla-inbound': {
        'repo_path': 'integration/mozilla-inbound',
    },
    'services-central': {
        'repo_path': 'services/services-central',
    },
    'ux': {
        "graph_server_branch_name": "UX",
    },
    
    
    'date': {
        'enable_release_promotion': 1,
        'platform_overrides': {
            'linux': {
                'src_mozconfig': 'browser/config/mozconfigs/linux32/beta',
            },
            'linux64': {
                'src_mozconfig': 'browser/config/mozconfigs/linux64/beta',
            },
            'macosx64': {
                'src_mozconfig': 'browser/config/mozconfigs/macosx-universal/beta',
            },
            'win32': {
                'src_mozconfig': 'browser/config/mozconfigs/win32/beta',
            },
            'win64': {
                'src_mozconfig': 'browser/config/mozconfigs/win64/beta',
            },
        },
    },
    'cypress': {
        
        'branch_uses_per_checkin_strategy': True,
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
}
