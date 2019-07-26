import os
from setuptools import setup, find_packages

version = '0.5.10'


try:
    here = os.path.dirname(os.path.abspath(__file__))
    description = file(os.path.join(here, 'README.md')).read()
except (OSError, IOError):
    description = ''


deps = ['manifestdestiny', 'mozhttpd >= 0.3',
        'mozprocess >= 0.6', 'mozrunner >= 5.11',
        'mozdevice >= 0.12']

setup(name='marionette_client',
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
      package_data={'marionette': ['touch/*.js']},
      include_package_data=True,
      zip_safe=False,
      entry_points="""
      # -*- Entry points: -*-
      [console_scripts]
      marionette = marionette.runtests:cli
      """,
      install_requires=deps,
      )

