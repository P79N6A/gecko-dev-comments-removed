



from setuptools import setup

PACKAGE_VERSION = '0.2'

setup(name='mozfile',
      version=PACKAGE_VERSION,
      description="Library of file utilities for use in Mozilla testing",
      long_description="see http://mozbase.readthedocs.org/",
      classifiers=[], 
      keywords='mozilla',
      author='Mozilla Automation and Tools team',
      author_email='tools@lists.mozilla.org',
      url='https://wiki.mozilla.org/Auto-tools/Projects/MozBase',
      license='MPL',
      packages=['mozfile'],
      include_package_data=True,
      zip_safe=False,
      install_requires=[]
      )
