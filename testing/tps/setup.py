




































import sys
from setuptools import setup, find_packages

version = '0.2.40'

deps = ['pulsebuildmonitor >= 0.2', 'MozillaPulse == .4', 
        'mozinfo == 0.3.1', 'mozprofile == 0.1t',
        'mozprocess == 0.1a', 'mozrunner == 3.0a', 'mozregression == 0.3',
        'mozautolog >= 0.2.1']


assert sys.version_info[0] == 2
assert sys.version_info[1] >= 6

setup(name='tps',
      version=version,
      description='run automated multi-profile sync tests',
      long_description="""\
""",
      classifiers=[], 
      keywords='',
      author='Jonathan Griffin',
      author_email='jgriffin@mozilla.com',
      url='http://hg.mozilla.org/services/services-central',
      license='MPL',
      dependency_links = [
         "http://people.mozilla.org/~jgriffin/packages/"
      ],
      packages=find_packages(exclude=['ez_setup', 'examples', 'tests']),
      include_package_data=True,
      zip_safe=False,
      install_requires=deps,
      entry_points="""
      # -*- Entry points: -*-
      [console_scripts]
      runtps = tps.cli:main
      """,
      data_files=[
        ('tps', ['config/config.json.in']),
      ],
      )
