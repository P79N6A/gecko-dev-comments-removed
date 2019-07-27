import os
from setuptools import setup, find_packages
import sys

version = '0.1.0'


with open('requirements.txt') as f:
    deps = f.read().splitlines()

setup(name='marionette_driver',
      version=version,
      description="Marionette Driver",
      long_description='See http://marionette-driver.readthedocs.org/',
      classifiers=[], 
      keywords='mozilla',
      author='Auto-tools',
      author_email='tools-marionette@lists.mozilla.org',
      url='https://wiki.mozilla.org/Auto-tools/Projects/Marionette',
      license='MPL',
      packages=find_packages(),
      
      include_package_data=True,
      zip_safe=False,
      install_requires=deps,
      )

