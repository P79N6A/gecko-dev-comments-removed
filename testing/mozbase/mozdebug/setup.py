



from setuptools import setup

PACKAGE_VERSION = '0.1'

setup(name='mozdebug',
      version=PACKAGE_VERSION,
      description="Utilities for running applications under native code debuggers intended for use in Mozilla testing",
      long_description="see http://mozbase.readthedocs.org/",
      classifiers=[], 
      keywords='mozilla',
      author='Mozilla Automation and Testing Team',
      author_email='tools@lists.mozilla.org',
      url='https://wiki.mozilla.org/Auto-tools/Projects/Mozbase',
      license='MPL',
      packages=['mozdebug'],
      include_package_data=True,
      zip_safe=False,
      install_requires=['mozinfo'],
      entry_points="""
      # -*- Entry points: -*-
      """,
      )

