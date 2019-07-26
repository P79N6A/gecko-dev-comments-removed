



from setuptools import setup

PACKAGE_VERSION = '0.7'


deps = ['mozfile >= 0.12']

setup(name='mozinfo',
      version=PACKAGE_VERSION,
      description="Library to get system information for use in Mozilla testing",
      long_description="see http://mozbase.readthedocs.org",
      classifiers=[], 
      keywords='mozilla',
      author='Mozilla Automation and Testing Team',
      author_email='tools@lists.mozilla.org',
      url='https://wiki.mozilla.org/Auto-tools/Projects/Mozbase',
      license='MPL',
      packages=['mozinfo'],
      include_package_data=True,
      zip_safe=False,
      install_requires=deps,
      entry_points="""
      # -*- Entry points: -*-
      [console_scripts]
      mozinfo = mozinfo:main
      """,
      )
