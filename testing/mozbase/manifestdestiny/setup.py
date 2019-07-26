



from setuptools import setup
import sys
import os

here = os.path.dirname(os.path.abspath(__file__))
try:
    filename = os.path.join(here, 'README.md')
    description = file(filename).read()
except:
    description = ''

PACKAGE_NAME = "ManifestDestiny"
PACKAGE_VERSION = '0.5.6'

setup(name=PACKAGE_NAME,
      version=PACKAGE_VERSION,
      description="Universal manifests for Mozilla test harnesses",
      long_description=description,
      classifiers=[], 
      keywords='mozilla manifests',
      author='Mozilla Automation and Testing Team',
      author_email='tools@lists.mozilla.org',
      url='https://wiki.mozilla.org/Auto-tools/Projects/MozBase',
      license='MPL',
      zip_safe=False,
      packages=['manifestparser'],
      install_requires=[],
      entry_points="""
      [console_scripts]
      manifestparser = manifestparser.manifestparser:main
      """,
     )
