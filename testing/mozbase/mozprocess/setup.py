






































import os
from setuptools import setup, find_packages

version = '0.1b2'


here = os.path.dirname(os.path.abspath(__file__))
try:
    description = file(os.path.join(here, 'README.md')).read()
except (OSError, IOError):
    description = ''

setup(name='mozprocess',
      version=version,
      description="Mozilla-authored process handling",
      long_description=description,
      classifiers=[], 
      keywords='',
      author='Mozilla Automation and Testing Team',
      author_email='tools@lists.mozilla.com',
      url='https://github.com/mozilla/mozbase/tree/master/mozprocess',
      license='MPL',
      packages=find_packages(exclude=['ez_setup', 'examples', 'tests']),
      include_package_data=True,
      zip_safe=False,
      install_requires=['mozinfo'],
      entry_points="""
      # -*- Entry points: -*-
      """,
      )
