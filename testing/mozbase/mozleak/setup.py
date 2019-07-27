



from setuptools import setup


PACKAGE_NAME = 'mozleak'
PACKAGE_VERSION = '0.1'


setup(
    name=PACKAGE_NAME,
    version=PACKAGE_VERSION,
    description="Library for extracting memory leaks from leak logs files",
    long_description="see http://mozbase.readthedocs.org/",
    classifiers=[], 
    keywords='mozilla',
    author='Mozilla Automation and Tools team',
    author_email='tools@lists.mozilla.org',
    url='https://wiki.mozilla.org/Auto-tools/Projects/Mozbase',
    license='MPL',
    packages=['mozleak'],
    zip_safe=False,
    install_requires=[],
)
