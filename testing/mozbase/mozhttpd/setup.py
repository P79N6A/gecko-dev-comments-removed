



from setuptools import setup

PACKAGE_VERSION = '0.6'
deps = ['moznetwork >= 0.1']

setup(name='mozhttpd',
      version=PACKAGE_VERSION,
      description="Python webserver intended for use with Mozilla testing",
      long_description="see http://mozbase.readthedocs.org/",
      classifiers=[], 
      keywords='mozilla',
      author='Mozilla Automation and Testing Team',
      author_email='tools@lists.mozilla.org',
      url='https://wiki.mozilla.org/Auto-tools/Projects/Mozbase',
      license='MPL',
      packages=['mozhttpd'],
      include_package_data=True,
      zip_safe=False,
      install_requires=deps,
      entry_points="""
      # -*- Entry points: -*-
      [console_scripts]
      mozhttpd = mozhttpd:main
      """,
      )

