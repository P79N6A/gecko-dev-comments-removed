



from setuptools import setup

VERSION = '0.1'

setup(
    name='mozbuild',
    description='Mozilla build system functionality.',
    license='MPL 2.0',
    packages=['mach', 'mozbuild', 'mozpack'],
    version=VERSION
)
