




import os
from setuptools import setup

PACKAGE_VERSION = '0.1'


try:
    here = os.path.dirname(os.path.abspath(__file__))
    description = file(os.path.join(here, 'README.md')).read()
except (OSError, IOError):
    description = ''


deps = ['']

setup(name='mozcrash',
      version=PACKAGE_VERSION,
      description="Package for printing stack traces from minidumps left behind by crashed processes.",
      long_description=description,
      classifiers=[], 
      keywords='mozilla',
      author='Mozilla Automation and Tools team',
      author_email='tools@lists.mozilla.org',
      url='https://wiki.mozilla.org/Auto-tools/Projects/MozBase',
      license='MPL',
      packages=['mozcrash'],
      include_package_data=True,
      zip_safe=False,
      install_requires=deps,
      )
