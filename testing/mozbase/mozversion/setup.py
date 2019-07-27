



from setuptools import setup

PACKAGE_VERSION = '1.2'

dependencies = ['mozdevice >= 0.44',
                'mozfile >= 1.0',
                'mozlog >= 2.11']

setup(name='mozversion',
      version=PACKAGE_VERSION,
      description='Library to get version information for applications',
      long_description='See http://mozbase.readthedocs.org',
      classifiers=[],
      keywords='mozilla',
      author='Mozilla Automation and Testing Team',
      author_email='tools@lists.mozilla.org',
      url='https://wiki.mozilla.org/Auto-tools/Projects/Mozbase',
      license='MPL',
      packages=['mozversion'],
      include_package_data=True,
      zip_safe=False,
      install_requires=dependencies,
      entry_points="""
      # -*- Entry points: -*-
      [console_scripts]
      mozversion = mozversion:cli
      """)
