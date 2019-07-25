import os
from setuptools import setup, find_packages

version = '0.3'


try:
    here = os.path.dirname(os.path.abspath(__file__))
    description = file(os.path.join(here, 'README.md')).read()
except (OSError, IOError):
    description = ''


deps = ['manifestdestiny', 'mozhttpd >= 0.3',
        'mozprocess == 0.5', 'mozrunner == 5.10', 'datazilla == 0.2.1']

setup(name='marionette',
      version=version,
      description="Marionette test automation client",
      long_description=description,
      classifiers=[], 
      keywords='mozilla',
      author='Jonathan Griffin',
      author_email='jgriffin@mozilla.com',
      url='https://wiki.mozilla.org/Auto-tools/Projects/Marionette',
      license='MPL',
      packages=find_packages(exclude=['ez_setup', 'examples', 'tests']),
      include_package_data=True,
      zip_safe=False,
      install_requires=deps,
      )

