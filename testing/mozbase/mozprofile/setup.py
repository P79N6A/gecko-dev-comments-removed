



import sys
from setuptools import setup

PACKAGE_NAME = 'mozprofile'
PACKAGE_VERSION = '0.22'


assert sys.version_info[0] == 2

deps = ['manifestparser >= 0.6',
        'mozfile >= 1.0',
        'mozlog']

setup(name=PACKAGE_NAME,
      version=PACKAGE_VERSION,
      description="Library to create and modify Mozilla application profiles",
      long_description="see http://mozbase.readthedocs.org/",
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
      author_email='tools@lists.mozilla.org',
      url='https://wiki.mozilla.org/Auto-tools/Projects/Mozbase',
      license='MPL 2.0',
      packages=['mozprofile'],
      include_package_data=True,
      zip_safe=False,
      install_requires=deps,
      tests_require=['mozhttpd'],
      entry_points="""
      # -*- Entry points: -*-
      [console_scripts]
      mozprofile = mozprofile:cli
      view-profile = mozprofile:view_profile
      diff-profiles = mozprofile:diff_profiles
      """,
    )
