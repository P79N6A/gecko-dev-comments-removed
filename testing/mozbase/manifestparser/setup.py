



from setuptools import setup

PACKAGE_NAME = "manifestparser"
PACKAGE_VERSION = '0.9'

setup(name=PACKAGE_NAME,
      version=PACKAGE_VERSION,
      description="Library to create and manage test manifests",
      long_description="see http://mozbase.readthedocs.org/",
      classifiers=[], 
      keywords='mozilla manifests',
      author='Mozilla Automation and Testing Team',
      author_email='tools@lists.mozilla.org',
      url='https://wiki.mozilla.org/Auto-tools/Projects/Mozbase',
      license='MPL',
      zip_safe=False,
      packages=['manifestparser'],
      install_requires=[],
      entry_points="""
      [console_scripts]
      manifestparser = manifestparser.cli:main
      """,
     )
