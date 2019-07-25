




































from setuptools import setup, find_packages
import sys

version = '0.1a'


assert sys.version_info[0] == 2

deps = []

if sys.version_info[1] < 6:
    deps.append('simplejson')

setup(name='mozprofile',
      version=version,
      description="handling of Mozilla XUL app profiles",
      long_description="""\
""",
      classifiers=[], 
      keywords='',
      author='Mozilla Automation + Testing Team',
      author_email='mozmill-dev@googlegroups.com',
      url='http://github.com/mozautomation/mozmill',
      license='MPL',
      packages=find_packages(exclude=['ez_setup', 'examples', 'tests']),
      include_package_data=True,
      zip_safe=False,
      install_requires=deps,
      entry_points="""
      # -*- Entry points: -*-
      
      [console_scripts]
      addon_id = mozprofile:print_addon_ids
      """,
      )
