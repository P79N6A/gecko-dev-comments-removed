



from setuptools import setup

PACKAGE_VERSION = '0.3'

deps = ['mozdevice >= 0.16', 'marionette_client >= 0.5.2']

setup(name='mozb2g',
      version=PACKAGE_VERSION,
      description="B2G specific code for device automation",
      long_description="see http://mozbase.readthedocs.org/",
      classifiers=[], 
      keywords='',
      author='Mozilla Automation and Testing Team',
      author_email='tools@lists.mozilla.org',
      url='https://wiki.mozilla.org/Auto-tools/Projects/Mozbase',
      license='MPL',
      packages=['mozb2g'],
      include_package_data=True,
      zip_safe=False,
      install_requires=deps
      )
