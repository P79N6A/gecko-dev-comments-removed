































"""Set up script for mod_pywebsocket.
"""


from distutils.core import setup, Extension
import sys


_PACKAGE_NAME = 'mod_pywebsocket'


_USE_FAST_MASKING = False

if sys.version < '2.3':
    print >> sys.stderr, '%s requires Python 2.3 or later.' % _PACKAGE_NAME
    sys.exit(1)

if _USE_FAST_MASKING:
    setup(ext_modules=[
                  Extension(
                          'mod_pywebsocket/_fast_masking',
                          ['mod_pywebsocket/fast_masking.i'],
                          swig_opts=['-c++'])])

setup(author='Yuzo Fujishima',
      author_email='yuzo@chromium.org',
      description='WebSocket extension for Apache HTTP Server.',
      long_description=(
              'mod_pywebsocket is an Apache HTTP Server extension for '
              'the WebSocket Protocol (RFC 6455). '
              'See mod_pywebsocket/__init__.py for more detail.'),
      license='See COPYING',
      name=_PACKAGE_NAME,
      packages=[_PACKAGE_NAME, _PACKAGE_NAME + '.handshake'],
      url='http://code.google.com/p/pywebsocket/',
      
      
      version='0.7.9',
      )



