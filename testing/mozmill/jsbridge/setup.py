




































import sys
from setuptools import setup, find_packages

desc = """Python to JavaScript bridge interface."""
summ = """A powerful and extensible Python to JavaScript bridge interface."""

PACKAGE_NAME = "jsbridge"
PACKAGE_VERSION = "2.4.1rc2"

requires = ['mozrunner == 2.5.2rc2']

if not sys.version.startswith('2.6'):
    requires.append('simplejson')

setup(name=PACKAGE_NAME,
      version=PACKAGE_VERSION,
      description=desc,
      long_description=summ,
      author='Mikeal Rogers, Mozilla',
      author_email='mikeal.rogers@gmail.com',
      url='http://github.com/mozautomation/mozmill',
      license='http://www.apache.org/licenses/LICENSE-2.0',
      packages=find_packages(exclude=['test']),
      include_package_data=True,
      package_data = {'': ['*.js', '*.css', '*.html', '*.txt', '*.xpi', '*.rdf', '*.xul', '*.jsm', '*.xml' 'extension'],},
      entry_points="""
          [console_scripts]
          jsbridge = jsbridge:cli
        """,
      platforms =['Any'],
      install_requires = requires,
      classifiers=['Development Status :: 4 - Beta',
                   'Environment :: Console',
                   'Intended Audience :: Developers',
                   'License :: OSI Approved :: Apache Software License',
                   'Operating System :: OS Independent',
                   'Topic :: Software Development :: Libraries :: Python Modules',
                  ]
     )
