import os
from setuptools import setup, find_packages

version = '0.1'


try:
    here = os.path.dirname(os.path.abspath(__file__))
    description = file(os.path.join(here, 'README.md')).read()
except (OSError, IOError):
    description = ''


deps = []

setup(name='marionette-transport',
      version=version,
      description="Transport layer for Marionette client",
      long_description=description,
      classifiers=[], 
      keywords='mozilla',
      author='Mozilla Automation and Tools Team',
      author_email='tools@lists.mozilla.org',
      url='https://developer.mozilla.org/en-US/docs/Marionette',
      license='MPL',
      packages=find_packages(exclude=['ez_setup', 'examples', 'tests']),
      package_data={},
      include_package_data=False,
      zip_safe=False,
      entry_points="""
      """,
      install_requires=deps,
      )

