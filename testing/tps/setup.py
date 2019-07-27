



from setuptools import setup, find_packages
import sys

version = '0.5'

deps = ['httplib2 == 0.7.3',
        'mozfile == 1.1',
        'mozhttpd == 0.7',
        'mozinfo == 0.7',
        'mozinstall == 1.10',
        'mozprocess == 0.19',
        'mozprofile == 0.21',
        'mozrunner == 6.0',
        'mozversion == 0.6',
       ]


assert sys.version_info[0] == 2
assert sys.version_info[1] >= 6

setup(name='tps',
      version=version,
      description='run automated multi-profile sync tests',
      long_description="""\
""",
      classifiers=[], 
      keywords='',
      author='Mozilla Automation and Tools team',
      author_email='tools@lists.mozilla.org',
      url='https://developer.mozilla.org/en-US/docs/TPS',
      license='MPL 2.0',
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
