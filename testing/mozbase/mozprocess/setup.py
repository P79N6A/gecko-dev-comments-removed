



from setuptools import setup

PACKAGE_VERSION = '0.14'

setup(name='mozprocess',
      version=PACKAGE_VERSION,
      description="Mozilla-authored process handling",
      long_description='see http://mozbase.readthedocs.org/',
      classifiers=['Environment :: Console',
                   'Intended Audience :: Developers',
                   'License :: OSI Approved :: Mozilla Public License 2.0 (MPL 2.0)',
                   'Natural Language :: English',
                   'Operating System :: OS Independent',
                   'Programming Language :: Python',
                   'Topic :: Software Development :: Libraries :: Python Modules',
                   ],
      keywords='mozilla',
      author='Mozilla Automation and Tools team',
      author_email='tools@lists.mozilla.org',
      url='https://wiki.mozilla.org/Auto-tools/Projects/Mozbase',
      license='MPL 2.0',
      packages=['mozprocess'],
      include_package_data=True,
      zip_safe=False,
      install_requires=['mozinfo'],
      entry_points="""
      # -*- Entry points: -*-
      """,
      )
