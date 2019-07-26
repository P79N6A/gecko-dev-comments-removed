



import os
import shutil
import tempfile
import urllib2
import zipfile
from distutils import dir_util
from manifestparser import ManifestParser
from xml.dom import minidom


AMO_API_VERSION = "1.5"

class AddonManager(object):
    """
    Handles all operations regarding addons in a profile including: installing and cleaning addons
    """

    def __init__(self, profile):
        """
        :param profile: the path to the profile for which we install addons
        """
        self.profile = profile

        
        
        self.installed_addons = []
        self.installed_manifests = []

        
        self._addon_dirs = []

        
        self.backup_dir = None

    def install_addons(self, addons=None, manifests=None):
        """
        Installs all types of addons

        :param addons: a list of addon paths to install
        :param manifest: a list of addon manifests to install
        """
        
        if addons:
            if isinstance(addons, basestring):
                addons = [addons]
            self.installed_addons.extend(addons)
            for addon in addons:
                self.install_from_path(addon)
        
        if manifests:
            if isinstance(manifests, basestring):
                manifests = [manifests]
            for manifest in manifests:
                self.install_from_manifest(manifest)
            self.installed_manifests.extend(manifests)

    def install_from_manifest(self, filepath):
        """
        Installs addons from a manifest
        :param filepath: path to the manifest of addons to install
        """
        manifest = ManifestParser()
        manifest.read(filepath)
        addons = manifest.get()

        for addon in addons:
            if '://' in addon['path'] or os.path.exists(addon['path']):
                self.install_from_path(addon['path'])
                continue

            
            locale = addon.get('amo_locale', 'en_US')

            query = 'https://services.addons.mozilla.org/' + locale + '/firefox/api/' + AMO_API_VERSION + '/'
            if 'amo_id' in addon:
                query += 'addon/' + addon['amo_id']                 
            else:
                query += 'search/' + addon['name'] + '/default/1'   
            install_path = AddonManager.get_amo_install_path(query)
            self.install_from_path(install_path)

    @classmethod
    def get_amo_install_path(self, query):
        """
        Get the addon xpi install path for the specified AMO query.

        :param query: query-documentation_

        .. _query-documentation: https://developer.mozilla.org/en/addons.mozilla.org_%28AMO%29_API_Developers%27_Guide/The_generic_AMO_API
        """
        response = urllib2.urlopen(query)
        dom = minidom.parseString(response.read())
        for node in dom.getElementsByTagName('install')[0].childNodes:
            if node.nodeType == node.TEXT_NODE:
                return node.data

    @classmethod
    def addon_details(cls, addon_path):
        """
        Returns a dictionary of details about the addon.

        :param addon_path: path to the addon directory

        Returns::

            {'id':      u'rainbow@colors.org', # id of the addon
             'version': u'1.4',                # version of the addon
             'name':    u'Rainbow',            # name of the addon
             'unpack':  False } # whether to unpack the addon
        """

        
        details = {
            'id': None,
            'unpack': False,
            'name': None,
            'version': None
        }

        def get_namespace_id(doc, url):
            attributes = doc.documentElement.attributes
            namespace = ""
            for i in range(attributes.length):
                if attributes.item(i).value == url:
                    if ":" in attributes.item(i).name:
                        
                        namespace = attributes.item(i).name.split(':')[1] + ":"
                        break
            return namespace

        def get_text(element):
            """Retrieve the text value of a given node"""
            rc = []
            for node in element.childNodes:
                if node.nodeType == node.TEXT_NODE:
                    rc.append(node.data)
            return ''.join(rc).strip()

        doc = minidom.parse(os.path.join(addon_path, 'install.rdf'))

        
        em = get_namespace_id(doc, "http://www.mozilla.org/2004/em-rdf#")
        rdf = get_namespace_id(doc, "http://www.w3.org/1999/02/22-rdf-syntax-ns#")

        description = doc.getElementsByTagName(rdf + "Description").item(0)
        for node in description.childNodes:
            
            entry = node.nodeName.replace(em, "")
            if entry in details.keys():
                details.update({ entry: get_text(node) })

        
        if isinstance(details['unpack'], basestring):
            details['unpack'] = details['unpack'].lower() == 'true'

        return details

    def install_from_path(self, path, unpack=False):
        """
        Installs addon from a filepath, url or directory of addons in the profile.

        :param path: url, path to .xpi, or directory of addons
        :param unpack: whether to unpack unless specified otherwise in the install.rdf
        """

        
        
        if '://' in path:
            response = urllib2.urlopen(path)
            fd, path = tempfile.mkstemp(suffix='.xpi')
            os.write(fd, response.read())
            os.close(fd)
            tmpfile = path
        else:
            tmpfile = None

        
        addons = [path]
        if not path.endswith('.xpi') and not os.path.exists(os.path.join(path, 'install.rdf')):
            
            if not os.path.isdir(path):
                return
            addons = [os.path.join(path, x) for x in os.listdir(path) if
                    os.path.isdir(os.path.join(path, x))]

        
        for addon in addons:
            tmpdir = None
            xpifile = None
            if addon.endswith('.xpi'):
                tmpdir = tempfile.mkdtemp(suffix = '.' + os.path.split(addon)[-1])
                compressed_file = zipfile.ZipFile(addon, 'r')
                for name in compressed_file.namelist():
                    if name.endswith('/'):
                        os.makedirs(os.path.join(tmpdir, name))
                    else:
                        if not os.path.isdir(os.path.dirname(os.path.join(tmpdir, name))):
                            os.makedirs(os.path.dirname(os.path.join(tmpdir, name)))
                        data = compressed_file.read(name)
                        f = open(os.path.join(tmpdir, name), 'wb')
                        f.write(data)
                        f.close()
                xpifile = addon
                addon = tmpdir

            
            addon_details = AddonManager.addon_details(addon)
            addon_id = addon_details.get('id')
            assert addon_id, 'The addon id could not be found: %s' % addon

            
            extensions_path = os.path.join(self.profile, 'extensions', 'staged')
            addon_path = os.path.join(extensions_path, addon_id)
            if not unpack and not addon_details['unpack'] and xpifile:
                if not os.path.exists(extensions_path):
                    os.makedirs(extensions_path)
                
                if os.path.exists(addon_path + '.xpi'):
                    self.backup_dir = self.backup_dir or tempfile.mkdtemp()
                    shutil.copy(addon_path + '.xpi', self.backup_dir)
                shutil.copy(xpifile, addon_path + '.xpi')
            else:
                
                if os.path.exists(addon_path):
                    self.backup_dir = self.backup_dir or tempfile.mkdtemp()
                    dir_util.copy_tree(addon_path, self.backup_dir, preserve_symlinks=1)
                dir_util.copy_tree(addon, addon_path, preserve_symlinks=1)
                self._addon_dirs.append(addon_path)

            
            if tmpdir:
                dir_util.remove_tree(tmpdir)

        
        if tmpfile:
            os.remove(tmpfile)

    def clean_addons(self):
        """Cleans up addons in the profile."""
        for addon in self._addon_dirs:
            if os.path.isdir(addon):
                dir_util.remove_tree(addon)
        
        if self.backup_dir and os.path.isdir(self.backup_dir):
            extensions_path = os.path.join(self.profile, 'extensions', 'staged')
            for backup in os.listdir(self.backup_dir):
                backup_path = os.path.join(self.backup_dir, backup)
                addon_path = os.path.join(extensions_path, addon)
                shutil.move(backup_path, addon_path)
            if not os.listdir(self.backup_dir):
                shutil.rmtree(self.backup_dir, ignore_errors=True)

    __del__ = clean_addons
