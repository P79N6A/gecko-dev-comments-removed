



import os
import sys
from setuptools import setup, find_packages

PACKAGE_NAME = "mozrunner"
PACKAGE_VERSION = '5.8'

desc = """Reliable start/stop/configuration of Mozilla Applications (Firefox, Thunderbird, etc.)"""

here = os.path.dirname(os.path.abspath(__file__))
try:
    description = file(os.path.join(here, 'README.md')).read()
except (OSError, IOError):
    description = ''

deps = ['mozinfo == 0.3.3',
        'mozprocess == 0.4',
        'mozprofile == 0.4',
       ]


assert sys.version_info[0] == 2

setup(name=PACKAGE_NAME,
      version=PACKAGE_VERSION,
      description=desc,
      long_description=description,
      classifiers=['Environment :: Console',
                   'Intended Audience :: Developers',
                   'License :: OSI Approved :: Mozilla Public License 2.0 (MPL 2.0)',
                   'Natural Language :: English',
                   'Operating System :: OS Independent',
                   'Programming Language :: Python',
                   'Topic :: Software Development :: Libraries :: Python Modules',
                   ],
      keywords='mozilla',
      author='Mozilla Automation and Tools team',
      author_email='tools@lists.mozilla.com',
      url='https://github.com/mozilla/mozbase/tree/master/mozrunner',
      license='MPL 2.0',
      packages=find_packages(exclude=['legacy']),
      zip_safe=False,
      install_requires = deps,
      entry_points="""
      # -*- Entry points: -*-
      [console_scripts]
      mozrunner = mozrunner:cli
      """,
    )
