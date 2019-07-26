



from setuptools import setup

PACKAGE_VERSION = '0.2'


deps = ['mozinfo']
try:
    import json
except ImportError:
    deps.append('simplejson')

setup(name='moztest',
      version=PACKAGE_VERSION,
      description="Package for storing and outputting Mozilla test results",
      long_description="see http://mozbase.readthedocs.org/",
      classifiers=[], 
      keywords='mozilla',
      author='Mozilla Automation and Tools team',
      author_email='tools@lists.mozilla.org',
      url='https://wiki.mozilla.org/Auto-tools/Projects/Mozbase',
      license='MPL',
      packages=['moztest'],
      include_package_data=True,
      zip_safe=False,
      install_requires=deps,
      )
