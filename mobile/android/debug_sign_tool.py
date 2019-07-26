





"""
Sign Android packages using an Android debug keystore, creating the
keystore if it does not exist.

This and |zip| can be combined to replace the Android |apkbuilder|
tool, which was deprecated in SDK r22.

Exits with code 0 if creating the keystore and every signing succeeded,
or with code 1 if any creation or signing failed.
"""

from argparse import ArgumentParser
import errno
import logging
import os
import subprocess
import sys


log = logging.getLogger(os.path.basename(__file__))
log.setLevel(logging.INFO)
sh = logging.StreamHandler(stream=sys.stdout)
sh.setFormatter(logging.Formatter('%(name)s: %(message)s'))
log.addHandler(sh)


class DebugKeystore:
    """
    A thin abstraction on top of an Android debug key store.
    """
    def __init__(self, keystore):
        self._keystore = os.path.abspath(os.path.expanduser(keystore))
        self._alias = 'debug'
        self.verbose = False
        self.keytool = 'keytool'
        self.jarsigner = 'jarsigner'

    @property
    def keystore(self):
        return self._keystore

    @property
    def alias(self):
        return self._alias

    def _ensure_keystore(self):
        if os.path.exists(self.keystore):
            if self.verbose:
                log.debug('Keystore exists at %s' % self.keystore)
        else:
            self.create_keystore()

    def create_keystore(self):
        try:
            path = os.path.dirname(self.keystore)
            os.makedirs(path)
        except OSError as exception:
            if exception.errno != errno.EEXIST:
                raise

        args = [ self.keytool,
                 '-genkey',
                 '-v',
                 '-keystore', self.keystore,
                 '-storepass', 'android',
                 '-alias', self.alias,
                 '-keypass', 'android',
                 '-dname', 'CN=Android Debug,O=Android,C=US',
                 '-keyalg', 'RSA',
                 '-validity', '365',
               ]
        subprocess.check_call(args)
        if self.verbose:
            log.info('Created keystore at %s' % self.keystore)

    def sign(self, apk):
        self._ensure_keystore()

        args = [ self.jarsigner,
                 '-digestalg', 'SHA1',
                 '-sigalg', 'MD5withRSA',
                 '-keystore', self.keystore,
                 '-storepass', 'android',
                 apk,
                 self.alias,
               ]
        subprocess.check_call(args)
        if self.verbose:
            log.info('Signed %s with keystore at %s' % (apk, self.keystore))


def parse_args(argv):
    parser = ArgumentParser(description='Sign Android packages using an Android debug keystore.')
    parser.add_argument('apks', nargs='+',
                        metavar='APK',
                        help='Android packages to be signed')
    parser.add_argument('-q', '--quiet',
                        dest='verbose',
                        default=True,
                        action='store_false',
                        help='quiet output')
    parser.add_argument('--keytool',
                        metavar='PATH',
                        default='keytool',
                        help='path to Java keytool')
    parser.add_argument('--jarsigner',
                        metavar='PATH',
                        default='jarsigner',
                        help='path to Java jarsigner')
    parser.add_argument('--keystore',
                        metavar='PATH',
                        default='~/.android/debug.keystore',
                        help='path to keystore (default: ~/.android/debug.keystore)')
    parser.add_argument('-f', '--force-create-keystore',
                        dest='force',
                        default=False,
                        action='store_true',
                        help='force creating keystore')
    return parser.parse_args(argv)


def main():
    args = parse_args(sys.argv[1:])

    keystore = DebugKeystore(args.keystore)
    keystore.verbose = args.verbose
    keystore.keytool = args.keytool
    keystore.jarsigner = args.jarsigner

    if args.force:
        try:
            keystore.create_keystore()
        except subprocess.CalledProcessError as e:
            log.error('Failed to force-create keystore')
            log.error(e)
            return 1

    for apk in args.apks:
        try:
            keystore.sign(apk)
        except subprocess.CalledProcessError as e:
            log.error('Failed to sign %s', apk)
            log.error(e)
            return 1

    return 0

if __name__ == '__main__':
    sys.exit(main())
