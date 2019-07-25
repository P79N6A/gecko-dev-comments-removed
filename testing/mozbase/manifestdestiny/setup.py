








from setuptools import setup, find_packages
import sys
import os

here = os.path.dirname(os.path.abspath(__file__))
try:
    filename = os.path.join(here, 'README.md')
    description = file(filename).read()
except:
    description = ''

PACKAGE_NAME = "ManifestDestiny"
PACKAGE_VERSION = "0.5.4"

setup(name=PACKAGE_NAME,
      version=PACKAGE_VERSION,
      description="Universal manifests for Mozilla test harnesses",
      long_description=description,
      classifiers=[], 
      keywords='mozilla manifests',
      author='Jeff Hammel',
      author_email='jhammel@mozilla.com',
      url='https://github.com/mozilla/mozbase/tree/master/manifestdestiny',
      license='MPL',
      zip_safe=False,
      packages=find_packages(exclude=['legacy']),
      install_requires=[
      
      ],
      entry_points="""
      [console_scripts]
      manifestparser = manifestparser:main
      """,
     )
