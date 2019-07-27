































"""Run all tests in the same directory.

This suite is expected to be run under pywebsocket's src directory, i.e. the
directory containing mod_pywebsocket, test, etc.

To change loggin level, please specify --log-level option.
    python test/run_test.py --log-level debug

To pass any option to unittest module, please specify options after '--'. For
example, run this for making the test runner verbose.
    python test/run_test.py --log-level debug -- -v
"""


import logging
import optparse
import os
import re
import sys
import unittest


_TEST_MODULE_PATTERN = re.compile(r'^(test_.+)\.py$')


def _list_test_modules(directory):
    module_names = []
    for filename in os.listdir(directory):
        match = _TEST_MODULE_PATTERN.search(filename)
        if match:
            module_names.append(match.group(1))
    return module_names


def _suite():
    loader = unittest.TestLoader()
    return loader.loadTestsFromNames(
            _list_test_modules(os.path.join(os.path.split(__file__)[0], '.')))


if __name__ == '__main__':
    parser = optparse.OptionParser()
    parser.add_option('--log-level', '--log_level', type='choice',
                      dest='log_level', default='warning',
                      choices=['debug', 'info', 'warning', 'warn', 'error',
                               'critical'])
    options, args = parser.parse_args()
    logging.basicConfig(
            level=logging.getLevelName(options.log_level.upper()),
            format='%(levelname)s %(asctime)s '
                   '%(filename)s:%(lineno)d] '
                   '%(message)s',
            datefmt='%H:%M:%S')
    unittest.main(defaultTest='_suite', argv=[sys.argv[0]] + args)



