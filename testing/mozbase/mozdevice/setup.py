



import os
from setuptools import setup

PACKAGE_VERSION = '0.9'


here = os.path.dirname(os.path.abspath(__file__))
try:
    description = file(os.path.join(here, 'README.md')).read()
except (OSError, IOError):
    description = ''

deps = ['mozprocess == 0.7']

setup(name='mozdevice',
      version=PACKAGE_VERSION,
      description="Mozilla-authored device management",
      long_description=description,
      classifiers=[], 
      keywords='',
      author='Mozilla Automation and Testing Team',
      author_email='tools@lists.mozilla.org',
      url='https://wiki.mozilla.org/Auto-tools/Projects/MozBase',
      license='MPL',
      packages=['mozdevice'],
      include_package_data=True,
      zip_safe=False,
      install_requires=deps,
      entry_points="""
      # -*- Entry points: -*-
      [console_scripts]
      sut = mozdevice.sutcli:cli
      """,
      )
