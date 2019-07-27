






from __future__ import print_function
import argparse
import os
import sys

from luciddream import LucidDreamTestCase
from marionette import Marionette
from marionette.runner import BaseMarionetteTestRunner
import marionette
from mozlog import structured


class CommandLineError(Exception):
    pass


def validate_options(options):
    if not (options.b2gPath or options.b2gDesktopPath):
        raise CommandLineError('You must specify --b2gpath or ' +
                               '--b2g-desktop-path')
    if options.b2gPath and options.b2gDesktopPath:
        raise CommandLineError('You may only use one of --b2gpath or ' +
                               '--b2g-desktop-path')
    if options.gaiaProfile and options.b2gPath:
        raise CommandLineError('You may not use --gaia-profile with ' +
                               '--b2gpath')
    if not options.browserPath:
        raise CommandLineError('You must specify --browser-path')
    if not os.path.isfile(options.manifest):
        raise CommandLineError('The manifest at "%s" does not exist!'
                               % options.manifest)





def parse_args(in_args):
    parser = argparse.ArgumentParser(description='Run Luciddream tests.')
    parser.add_argument('--emulator', dest='emulator', action='store',
                        default='arm',
                        help='Architecture of emulator to use: x86 or arm')
    parser.add_argument('--b2gpath', dest='b2gPath', action='store',
                        help='path to B2G repo or qemu dir')
    parser.add_argument('--b2g-desktop-path', dest='b2gDesktopPath',
                        action='store',
                        help='path to B2G desktop binary')
    parser.add_argument('--browser-path', dest='browserPath', action='store',
                        help='path to Firefox binary')
    parser.add_argument('--gaia-profile', dest='gaiaProfile', action='store',
                        help='path to Gaia profile')
    parser.add_argument('manifest', metavar='MANIFEST', action='store',
                        help='path to manifest of tests to run')
    structured.commandline.add_logging_group(parser)

    args = parser.parse_args(in_args)
    try:
        validate_options(args)
        return args
    except CommandLineError as e:
        print('Error: ', e.args[0], file=sys.stderr)
        parser.print_help()
        raise


class LucidDreamTestRunner(BaseMarionetteTestRunner):
    def __init__(self, **kwargs):
        BaseMarionetteTestRunner.__init__(self, **kwargs)
        
        self.test_handlers = [LucidDreamTestCase]


def start_browser(browserPath, app_args):
    '''
    Start a Firefox browser and return a Marionette instance that
    can talk to it.
    '''
    marionette = Marionette(
        bin=browserPath,
        
        
        port=2929,
        app_args=app_args,
        gecko_log="firefox.log"
    )
    runner = marionette.runner
    if runner:
        runner.start()
    marionette.wait_for_port()
    marionette.start_session()
    marionette.set_context(marionette.CONTEXT_CHROME)
    return marionette




def main(firefox=None, b2g_desktop=None, emulator=None, emulator_arch=None, gaia_profile=None, manifest=None, browser_args=None, **kwargs):

    
    
    
    
    
    
    browser = start_browser(firefox, browser_args)

    kwargs["browser"] = browser
    if not "logger" in kwargs:
        logger = structured.commandline.setup_logging(
            "luciddream", kwargs, {"tbpl": sys.stdout})
        kwargs["logger"] = logger

    if emulator:
        kwargs['homedir'] = emulator
        kwargs['emulator'] = emulator_arch
    elif b2g_desktop:
        
        if '-bin' not in b2g_desktop:
            if b2g_desktop.endswith('.exe'):
                newpath = b2g_desktop[:-4] + '-bin.exe'
            else:
                newpath = b2g_desktop + '-bin'
            if os.path.exists(newpath):
                b2g_desktop = newpath
        kwargs['binary'] = b2g_desktop
        kwargs['app'] = 'b2gdesktop'
        if gaia_profile:
            kwargs['profile'] = gaia_profile
        else:
            kwargs['profile'] = os.path.join(
                os.path.dirname(b2g_desktop),
                'gaia',
                'profile'
            )
    runner = LucidDreamTestRunner(**kwargs)
    runner.run_tests([manifest])
    if runner.failed > 0:
        sys.exit(10)
    sys.exit(0)


if __name__ == '__main__':
    args = parse_args(sys.argv[1:])
    main(args)
