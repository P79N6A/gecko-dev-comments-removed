



from setuptools import setup

PACKAGE_VERSION = '0.7'


deps = ['mozfile >= 0.3',
        'mozlog']

setup(name='mozcrash',
      version=PACKAGE_VERSION,
      description="Library for printing stack traces from minidumps left behind by crashed processes",
      long_description="see http://mozbase.readthedocs.org/",
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
