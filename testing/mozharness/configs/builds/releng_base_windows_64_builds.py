import os
import sys

config = {
    
    
    
    
    
    

    'default_actions': [
        'clobber',
        'clone-tools',
        'checkout-sources',
        
        'build',
        'upload-files',
        'sendchange',
        'check-test',
        'generate-build-stats',
        'update',  
    ],
    "buildbot_json_path": "buildprops.json",
    'exes': {
        'python2.7': sys.executable,
        'hgtool.py': [
            sys.executable,
            os.path.join(
                os.getcwd(), 'build', 'tools', 'buildfarm', 'utils', 'hgtool.py'
            )
        ],
        "buildbot": {
            'gpo_location': [
                sys.executable,
                'c:\\mozilla-build\\buildbotve\\scripts\\buildbot'
            ],
            'puppet_location': [
                sys.executable,
                'c:\\mozilla-build\\buildbot-0.8.4-pre-moz6\\scripts\\buildbot'
            ]
        },
        "make": [
            sys.executable,
            os.path.join(
                os.getcwd(), 'build', 'src', 'build', 'pymake', 'make.py'
            )
        ],
        'virtualenv': [
            sys.executable,
            'c:/mozilla-build/buildbotve/virtualenv.py'
        ],
    },
    'app_ini_path': '%(obj_dir)s/dist/bin/application.ini',
    
    'enable_signing': True,
    'purge_skip': ['info', 'rel-*:45d', 'tb-rel-*:45d'],
    'purge_basedirs':  [],
    'enable_ccache': False,
    'vcs_share_base': 'C:/builds/hg-shared',
    'objdir': 'obj-firefox',
    'tooltool_script': [sys.executable,
                        'C:/mozilla-build/tooltool.py'],
    'tooltool_bootstrap': "setup.sh",
    'enable_count_ctors': False,
    'enable_talos_sendchange': True,
    'enable_unittest_sendchange': True,
    'max_build_output_timeout': 60 * 80,
    


     
     
    'base_name': 'WINNT_6.1_x86-64_%(branch)s',
    'platform': 'win64',
    'stage_platform': 'win64',
    'env': {
        'MOZ_SYMBOLS_EXTRA_BUILDID': 'win64',
        'MOZ_AUTOMATION': '1',
        'HG_SHARE_BASE_DIR': 'C:/builds/hg-shared',
        'MOZ_CRASHREPORTER_NO_REPORT': '1',
        'MOZ_OBJDIR': 'obj-firefox',
        'PATH': 'C:/mozilla-build/python27;C:/mozilla-build/buildbotve/scripts;'
                '%s' % (os.environ.get('path')),
        'PDBSTR_PATH': '/c/Program Files (x86)/Windows Kits/8.0/Debuggers/x64/srcsrv/pdbstr.exe',
        'PROPERTIES_FILE': os.path.join(os.getcwd(), 'buildprops.json'),
        
        'SYMBOL_SERVER_HOST': '%(symbol_server_host)s',
        'SYMBOL_SERVER_SSH_KEY': '/c/Users/cltbld/.ssh/ffxbld_rsa',
        'SYMBOL_SERVER_USER': 'ffxbld',
        'SYMBOL_SERVER_PATH': '/mnt/netapp/breakpad/symbols_ffx/',
        'POST_SYMBOL_UPLOAD_CMD': '/usr/local/bin/post-symbol-upload.py',
        'TINDERBOX_OUTPUT': '1',
    },
    'upload_env': {
        
        'UPLOAD_HOST': '%(stage_server)s',
        'UPLOAD_USER': '%(stage_username)s',
        'UPLOAD_SSH_KEY': '/c/Users/cltbld/.ssh/%(stage_ssh_key)s',
        'UPLOAD_TO_TEMP': '1',
    },
    "check_test_env": {
        'MINIDUMP_STACKWALK': '%(abs_tools_dir)s/breakpad/win64/minidump_stackwalk.exe',
        'MINIDUMP_SAVE_PATH': '%(base_work_dir)s/minidumps',
    },
    'enable_pymake': True,
    'purge_minsize': 12,
    'src_mozconfig': 'browser/config/mozconfigs/win64/nightly',
    'tooltool_manifest_src': "browser/config/tooltool-manifests/win64/releng.manifest",
    
}
