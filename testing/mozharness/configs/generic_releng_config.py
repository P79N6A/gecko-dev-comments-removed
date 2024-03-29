

from mozharness.base.script import platform_name


PYTHON_WIN32 = 'c:/mozilla-build/python27/python.exe'


PLATFORM_CONFIG = {
    'linux64': {
        'exes': {
            'gittool.py': '/usr/local/bin/gittool.py',
            'hgtool.py': '/usr/local/bin/hgtool.py',
            'python': '/tools/buildbot/bin/python',
            'virtualenv': ['/tools/buildbot/bin/python', '/tools/misc-python/virtualenv.py'],
        },
        'env': {
            'DISPLAY': ':2',
        }
    },
    'macosx': {
        'exes': {
            'gittool.py': '/usr/local/bin/gittool.py',
            'hgtool.py': '/usr/local/bin/hgtool.py',
            'python': '/tools/buildbot/bin/python',
            'virtualenv': ['/tools/buildbot/bin/python', '/tools/misc-python/virtualenv.py'],
        },
    },
    'win32': {
        "exes": {
            'gittool.py': [PYTHON_WIN32, 'c:/builds/hg-shared/build/tools/buildfarm/utils/gittool.py'],
            'hgtool.py': [PYTHON_WIN32, 'c:/builds/hg-shared/build/tools/buildfarm/utils/hgtool.py'],
            
            'python': PYTHON_WIN32,
            'virtualenv': [PYTHON_WIN32, 'c:/mozilla-build/buildbotve/virtualenv.py'],
        }
    }
}

config = PLATFORM_CONFIG[platform_name()]

config.update({
    "find_links": [
        "http://pypi.pvt.build.mozilla.org/pub",
        "http://pypi.pub.build.mozilla.org/pub",
    ],
    'pip_index': False,
    'virtualenv_path': 'venv',
})

