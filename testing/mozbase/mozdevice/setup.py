



from setuptools import setup

PACKAGE_NAME = 'mozdevice'
PACKAGE_VERSION = '0.39'

deps = ['mozfile >= 1.0',
        'mozlog >= 2.1',
        'moznetwork >= 0.24',
        'mozprocess >= 0.19',
       ]

setup(name=PACKAGE_NAME,
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
      install_requires=deps,
      entry_points="""
      # -*- Entry points: -*-
      [console_scripts]
      dm = mozdevice.dmcli:cli
      sutini = mozdevice.sutini:main
      """,
      )
