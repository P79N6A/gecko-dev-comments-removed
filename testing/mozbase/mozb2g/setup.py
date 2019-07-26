



import os
from setuptools import setup

PACKAGE_VERSION = '0.1'


here = os.path.dirname(os.path.abspath(__file__))
try:
    description = file(os.path.join(here, 'README.md')).read()
except (OSError, IOError):
    description = ''

deps = ['mozdevice', 'marionette_client']

setup(name='mozb2g',
      version=PACKAGE_VERSION,
      description="B2G specific code for device automation",
      long_description=description,
      classifiers=[], 
      keywords='',
      author='Mozilla Automation and Testing Team',
      author_email='tools@lists.mozilla.org',
      url='https://wiki.mozilla.org/Auto-tools/Projects/MozBase',
      license='MPL',
      packages=['mozb2g'],
      include_package_data=True,
      zip_safe=False,
      install_requires=deps
      )
