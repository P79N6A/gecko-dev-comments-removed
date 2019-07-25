




































from setuptools import setup, find_packages

desc = """UI Automation tool for Mozilla applications."""
summ = """A tool for full UI automation of Mozilla applications."""

PACKAGE_NAME = "mozmill"
PACKAGE_VERSION = "1.4.1"

setup(name=PACKAGE_NAME,
      version=PACKAGE_VERSION,
      description=desc,
      long_description=summ,
      author='Mozilla, Mikeal Rogers',
      author_email='mikeal.rogers@gmail.com',
      url='http://code.google.com/p/mozmill',
      license='http://www.apache.org/licenses/LICENSE-2.0',
      packages=find_packages(exclude=['test']),
      include_package_data=True,
      package_data = {'': ['*.js', '*.css', '*.html', '*.txt', '*.xpi', '*.rdf', '*.xul', '*.jsm', '*.xml'],},
      entry_points="""
          [console_scripts]
          mozmill = mozmill:cli
          mozmill-thunderbird = mozmill:tbird_cli
          mozmill-restart = mozmill:restart_cli
        """,
      platforms =['Any'],
      install_requires = ['jsbridge >= 2.3.5', 'mozrunner >= 2.4.2'],
      classifiers=['Development Status :: 4 - Beta',
                   'Environment :: Console',
                   'Intended Audience :: Developers',
                   'License :: OSI Approved :: Apache Software License',
                   'Operating System :: OS Independent',
                   'Topic :: Software Development :: Libraries :: Python Modules',
                  ]
     )
