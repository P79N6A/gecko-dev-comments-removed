



import ConfigParser
from StringIO import StringIO
from optparse import OptionParser
import os
import re
import sys
import tempfile
import xml.dom.minidom
import zipfile

import mozdevice
import mozlog
import mozfile


class VersionError(Exception):
    def __init__(self, message):
        Exception.__init__(self, message)


class LocalAppNotFoundError(VersionError):
    """Exception for local application not found"""
    def __init__(self):
        VersionError.__init__(
            self, 'No binary path or application.ini found in working '
            'directory. Specify a binary path or run from the directory '
            'containing the binary.')


class Version(mozlog.LoggingMixin):

    def __init__(self):
        self._info = {}

    def get_gecko_info(self, config_path):
        for filename, section in (('application', 'App'),
                                  ('platform', 'Build')):
            config = ConfigParser.RawConfigParser()
            config_file = os.path.join(config_path, '%s.ini' % filename)
            if os.path.exists(config_file):
                config.read(config_file)
                for key in ('BuildID', 'Name', 'Version', 'SourceRepository',
                            'SourceStamp'):
                    name_map = {'SourceRepository': 'repository',
                                'SourceStamp': 'changeset'}
                    name = name_map.get(key, key).lower()
                    self._info['%s_%s' % (filename, name)] = config.has_option(
                        section, key) and config.get(section, key) or None
            else:
                self.warn('Unable to find %s' % config_file)


class LocalVersion(Version):

    def __init__(self, binary, **kwargs):
        Version.__init__(self, **kwargs)
        path = None

        if binary:
            if not os.path.exists(binary):
                raise IOError('Binary path does not exist: %s' % binary)
            path = os.path.dirname(binary)
        else:
            if os.path.exists(os.path.join(os.getcwd(), 'application.ini')):
                path = os.getcwd()

        if not path:
            raise LocalAppNotFoundError()

        self.get_gecko_info(path)


class B2GVersion(Version):

    def __init__(self, sources=None, **kwargs):
        Version.__init__(self, **kwargs)

        sources = sources or \
            os.path.exists(os.path.join(os.getcwd(), 'sources.xml')) and \
            os.path.join(os.getcwd(), 'sources.xml')

        if sources:
            sources_xml = xml.dom.minidom.parse(sources)
            for element in sources_xml.getElementsByTagName('project'):
                path = element.getAttribute('path')
                changeset = element.getAttribute('revision')
                if path in ['gaia', 'gecko', 'build']:
                    if path == 'gaia' and self._info.get('gaia_changeset'):
                        break
                    self._info['_'.join([path, 'changeset'])] = changeset

    def get_gaia_info(self, app_zip):
        with app_zip.open('resources/gaia_commit.txt') as f:
            changeset, date = f.read().splitlines()
            self._info['gaia_changeset'] = re.match('^\w{40}$', changeset) and \
                changeset or None
            self._info['gaia_date'] = date


class LocalB2GVersion(B2GVersion):

    def __init__(self, binary, sources=None, **kwargs):
        B2GVersion.__init__(self, sources, **kwargs)

        if binary:
            if not os.path.exists(binary):
                raise IOError('Binary path does not exist: %s' % binary)
            path = os.path.dirname(binary)
        else:
            if os.path.exists(os.path.join(os.getcwd(), 'application.ini')):
                path = os.getcwd()

        self.get_gecko_info(path)

        zip_path = os.path.join(
            path, 'gaia', 'profile', 'webapps',
            'settings.gaiamobile.org', 'application.zip')
        if os.path.exists(zip_path):
            zip_file = zipfile.ZipFile(zip_path)
            self.get_gaia_info(zip_file)
        else:
            self.warn('Error pulling gaia file')


class RemoteB2GVersion(B2GVersion):

    def __init__(self, sources=None, dm_type='adb', host=None, **kwargs):
        B2GVersion.__init__(self, sources, **kwargs)

        if dm_type == 'adb':
            dm = mozdevice.DeviceManagerADB()
        elif dm_type == 'sut':
            if not host:
                raise Exception('A host for SUT must be supplied.')
            dm = mozdevice.DeviceManagerSUT(host=host)
        else:
            raise Exception('Unknown device manager type: %s' % dm_type)

        if not sources:
            path = 'system/sources.xml'
            if dm.fileExists(path):
                sources = StringIO(dm.pullFile(path))
            else:
                self.info('Unable to find %s' % path)

        tempdir = tempfile.mkdtemp()
        for ini in ('application', 'platform'):
            with open(os.path.join(tempdir, '%s.ini' % ini), 'w') as f:
                f.write(dm.pullFile('/system/b2g/%s.ini' % ini))
                f.flush()
        self.get_gecko_info(tempdir)
        mozfile.remove(tempdir)

        for path in ['/system/b2g', '/data/local']:
            path += '/webapps/settings.gaiamobile.org/application.zip'
            if dm.fileExists(path):
                zip_file = zipfile.ZipFile(StringIO(dm.pullFile(path)))
                self.get_gaia_info(zip_file)
                break
        else:
            self.warn('Error pulling gaia file')

        build_props = dm.pullFile('/system/build.prop')
        desired_props = {
            'ro.build.version.incremental': 'device_firmware_version_incremental',
            'ro.build.version.release': 'device_firmware_version_release',
            'ro.build.date.utc': 'device_firmware_date',
            'ro.product.device': 'device_id'}
        for line in build_props.split('\n'):
            if not line.strip().startswith('#') and '=' in line:
                key, value = [s.strip() for s in line.split('=', 1)]
                if key in desired_props.keys():
                    self._info[desired_props[key]] = value


def get_version(binary=None, sources=None, dm_type=None, host=None):
    """
    Returns the application version information as a dict. If binary path is
    omitted then the current directory is checked for the existance of an
    application.ini file. If not found, then it is assumed the target
    application is a remote Firefox OS instance.

    :param binary: Path to the binary for the application
    :param sources: Path to the sources.xml file (Firefox OS)
    :param dm_type: Device manager type. Must be 'adb' or 'sut' (Firefox OS)
    :param host: Host address of remote Firefox OS instance (SUT)
    """
    try:
        version = LocalVersion(binary)
        if version._info.get('application_name') == 'B2G':
            version = LocalB2GVersion(binary, sources=sources)
    except LocalAppNotFoundError:
        version = RemoteB2GVersion(sources=sources, dm_type=dm_type, host=host)
    return version._info


def cli(args=sys.argv[1:]):
    parser = OptionParser()
    parser.add_option('--binary',
                      dest='binary',
                      help='path to application binary')
    parser.add_option('--sources',
                      dest='sources',
                      help='path to sources.xml (Firefox OS only)')
    (options, args) = parser.parse_args(args)

    dm_type = os.environ.get('DM_TRANS', 'adb')
    host = os.environ.get('TEST_DEVICE')

    version = get_version(binary=options.binary, sources=options.sources,
                          dm_type=dm_type, host=host)
    for (key, value) in sorted(version.items()):
        if value:
            print '%s: %s' % (key, value)

if __name__ == '__main__':
    cli()
