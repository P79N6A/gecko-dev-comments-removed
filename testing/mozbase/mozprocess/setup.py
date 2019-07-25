



import os
from setuptools import setup, find_packages

PACKAGE_VERSION = '0.4'


here = os.path.dirname(os.path.abspath(__file__))
try:
    description = file(os.path.join(here, 'README.md')).read()
except (OSError, IOError):
    description = ''

setup(name='mozprocess',
      version=PACKAGE_VERSION,
      description="Mozilla-authored process handling",
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
      url='https://github.com/mozilla/mozbase/tree/master/mozprocess',
      license='MPL 2.0',
      packages=find_packages(exclude=['ez_setup', 'examples', 'tests']),
      include_package_data=True,
      zip_safe=False,
      install_requires=['mozinfo'],
      entry_points="""
      # -*- Entry points: -*-
      """,
      )
