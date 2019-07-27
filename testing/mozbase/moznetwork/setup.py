



from setuptools import setup

PACKAGE_VERSION = '0.24'

deps=[ 'mozinfo' ]

setup(name='moznetwork',
      version=PACKAGE_VERSION,
      description="Library of network utilities for use in Mozilla testing",
      long_description="see http://mozbase.readthedocs.org/",
      classifiers=[], 
      keywords='mozilla',
      author='Mozilla Automation and Tools team',
      author_email='tools@lists.mozilla.org',
      url='https://wiki.mozilla.org/Auto-tools/Projects/Mozbase',
      license='MPL',
      packages=['moznetwork'],
      include_package_data=True,
      zip_safe=False,
      install_requires=deps,
      entry_points={'console_scripts': [
          'moznetwork = moznetwork:cli']},
      )
