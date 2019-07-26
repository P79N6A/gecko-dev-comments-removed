



import sys
from setuptools import setup, find_packages

version = '0.3'

deps = ['mozinfo >= 0.3.3', 'mozprofile >= 0.4',
        'mozprocess >= 0.4', 'mozrunner >= 5.8', 'mozinstall >= 1.4',
        'mozautolog >= 0.2.4', 'mozautoeslib >= 0.1.1', 'httplib2 >= 0.7.3']


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
