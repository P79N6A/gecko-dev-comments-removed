



import sys

from marionette import __version__
from marionette_driver import __version__ as driver_version
from marionette_transport import __version__ as transport_version
from marionette.marionette_test import MarionetteTestCase, MarionetteJSTestCase
from marionette.runner import (
    BaseMarionetteTestRunner,
    BaseMarionetteOptions,
    BrowserMobProxyOptionsMixin
)
import mozlog


class MarionetteTestRunner(BaseMarionetteTestRunner):
    def __init__(self, **kwargs):
        BaseMarionetteTestRunner.__init__(self, **kwargs)
        self.test_handlers = [MarionetteTestCase, MarionetteJSTestCase]


class MarionetteOptions(BaseMarionetteOptions,
                        BrowserMobProxyOptionsMixin):
    def __init__(self, **kwargs):
        BaseMarionetteOptions.__init__(self, **kwargs)
        BrowserMobProxyOptionsMixin.__init__(self, **kwargs)


def startTestRunner(runner_class, options, tests):
    if options.pydebugger:
        MarionetteTestCase.pydebugger = __import__(options.pydebugger)

    runner = runner_class(**vars(options))
    runner.run_tests(tests)
    return runner

def cli(runner_class=MarionetteTestRunner, parser_class=MarionetteOptions):
    parser = parser_class(
        usage='%prog [options] test_file_or_dir <test_file_or_dir> ...',
        version="%prog {version} (using marionette-driver: {driver_version}"
                ", marionette-transport: {transport_version})".format(
                    version=__version__,
                    driver_version=driver_version,
                    transport_version=transport_version)
    )
    mozlog.commandline.add_logging_group(parser)
    options, tests = parser.parse_args()
    parser.verify_usage(options, tests)

    logger = mozlog.commandline.setup_logging(
        options.logger_name, options, {"tbpl": sys.stdout})

    options.logger = logger

    runner = startTestRunner(runner_class, options, tests)
    if runner.failed > 0:
        sys.exit(10)

if __name__ == "__main__":
    cli()
