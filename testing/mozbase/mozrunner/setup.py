







































import os
import sys
from setuptools import setup, find_packages

PACKAGE_NAME = "mozrunner"
PACKAGE_VERSION = "4.0"

desc = """Reliable start/stop/configuration of Mozilla Applications (Firefox, Thunderbird, etc.)"""

here = os.path.dirname(os.path.abspath(__file__))
try:
    description = file(os.path.join(here, 'README.md')).read()
except (OSError, IOError):
    description = ''

deps = ['mozprocess', 'mozprofile', 'mozinfo']


assert sys.version_info[0] == 2

setup(name=PACKAGE_NAME,
      version=PACKAGE_VERSION,
      description=desc,
      long_description=description,
      author='Mikeal Rogers, Mozilla',
      author_email='mikeal.rogers@gmail.com',
      url='http://github.com/mozautomation/mozmill',
      license='MPL 1.1/GPL 2.0/LGPL 2.1',
      packages=find_packages(exclude=['legacy']),
      zip_safe=False,
      entry_points="""
          [console_scripts]
          mozrunner = mozrunner:cli
        """,
      platforms =['Any'],
      install_requires = deps,
      classifiers=['Development Status :: 4 - Beta',
                   'Environment :: Console',
                   'Intended Audience :: Developers',
                   'License :: OSI Approved :: Mozilla Public License 1.1 (MPL 1.1)',
                   'Operating System :: OS Independent',
                   'Topic :: Software Development :: Libraries :: Python Modules',
                  ]
     )
