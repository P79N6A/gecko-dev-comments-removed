






import os
import sys


sys.path.insert(1, os.path.dirname(sys.path[0]))

from mozharness.mozilla.testing.gaia_test import GaiaTest
from mozharness.mozilla.testing.unittest import TestSummaryOutputParserHelper


class GaiaBuildUnitTest(GaiaTest):

    def __init__(self, require_config_file=False):
      GaiaTest.__init__(self, require_config_file)

    def run_tests(self):
        """
        Run the gaia build unit test suite.
        """
        dirs = self.query_abs_dirs()

        self.node_setup()

        output_parser = TestSummaryOutputParserHelper(
          config=self.config, log_obj=self.log_obj, error_list=self.error_list)

        code = self.run_command([
            'make',
            'build-test-unit',
            'REPORTER=mocha-tbpl-reporter',
            'NODE_MODULES_SRC=npm-cache',
            'VIRTUALENV_EXISTS=1',
            'TRY_ENV=1'
        ], cwd=dirs['abs_gaia_dir'],
           output_parser=output_parser,
           output_timeout=330)

        output_parser.print_summary('gaia-build-unit-tests')
        self.publish(code)

if __name__ == '__main__':
    gaia_build_unit_test = GaiaBuildUnitTest()
    gaia_build_unit_test.run_and_exit()
