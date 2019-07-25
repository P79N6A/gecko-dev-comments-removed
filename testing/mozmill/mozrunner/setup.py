




































from setuptools import setup, find_packages
import sys

desc = """Reliable start/stop/configuration of Mozilla Applications (Firefox, Thunderbird, etc.)"""

PACKAGE_NAME = "mozrunner"
PACKAGE_VERSION = "2.5.2rc2"

deps = []


assert sys.version_info[0] == 2


if sys.version_info[1] < 6:
    deps.append('simplejson')

setup(name=PACKAGE_NAME,
      version=PACKAGE_VERSION,
      description=desc,
      long_description=desc,
      author='Mikeal Rogers, Mozilla',
      author_email='mikeal.rogers@gmail.com',
      url='http://github.com/mozautomation/mozmill',
      license='MPL 1.1/GPL 2.0/LGPL 2.1',
      packages=find_packages(exclude=['legacy']),
      entry_points="""
          [console_scripts]
          mozrunner = mozrunner:cli
          addon_id = mozrunner:print_addon_ids
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
