



from setuptools import setup

PACKAGE_NAME = 'mozfile'
PACKAGE_VERSION = '1.1'

setup(name=PACKAGE_NAME,
      version=PACKAGE_VERSION,
      description="Library of file utilities for use in Mozilla testing",
      long_description="see http://mozbase.readthedocs.org/",
      classifiers=[], 
      keywords='mozilla',
      author='Mozilla Automation and Tools team',
      author_email='tools@lists.mozilla.org',
      url='https://wiki.mozilla.org/Auto-tools/Projects/Mozbase',
      license='MPL',
      packages=['mozfile'],
      include_package_data=True,
      zip_safe=False,
      install_requires=[],
      tests_require=['mozhttpd']
      )
