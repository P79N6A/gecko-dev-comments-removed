



from setuptools import setup

PACKAGE_VERSION = '0.29'

setup(name='mozdevice',
      version=PACKAGE_VERSION,
      description="Mozilla-authored device management",
      long_description="see http://mozbase.readthedocs.org/",
      classifiers=[], 
      keywords='',
      author='Mozilla Automation and Testing Team',
      author_email='tools@lists.mozilla.org',
      url='https://wiki.mozilla.org/Auto-tools/Projects/Mozbase',
      license='MPL',
      packages=['mozdevice'],
      include_package_data=True,
      zip_safe=False,
      install_requires=['mozlog'],
      entry_points="""
      # -*- Entry points: -*-
      [console_scripts]
      dm = mozdevice.dmcli:cli
      sutini = mozdevice.sutini:main
      """,
      )
