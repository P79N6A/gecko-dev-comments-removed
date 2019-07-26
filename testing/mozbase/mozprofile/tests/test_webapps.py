

"""
test installing and managing webapps in a profile
"""

import os
import shutil
import unittest
from tempfile import mkdtemp

from mozprofile.webapps import WebappCollection, Webapp, WebappFormatException

here = os.path.dirname(os.path.abspath(__file__))

class WebappTest(unittest.TestCase):
    """Tests reading, installing and cleaning webapps
    from a profile.
    """
    manifest_path_1 = os.path.join(here, 'files', 'webapps1.json')
    manifest_path_2 = os.path.join(here, 'files', 'webapps2.json')

    def setUp(self):
        self.profile = mkdtemp(prefix='test_webapp')
        self.webapps_dir = os.path.join(self.profile, 'webapps')
        self.webapps_json_path = os.path.join(self.webapps_dir, 'webapps.json')

    def tearDown(self):
        shutil.rmtree(self.profile)

    def test_read_json_manifest(self):
        """Tests WebappCollection.read_json"""
        
        manifest_json_1 = WebappCollection.read_json(self.manifest_path_1)
        self.assertEqual(len(manifest_json_1), 7)
        for app in manifest_json_1:
            self.assertIsInstance(app, Webapp)
            for key in Webapp.required_keys:
                self.assertIn(key, app)

        
        manifest_json_2 = WebappCollection.read_json(self.manifest_path_2)
        self.assertEqual(len(manifest_json_2), 5)
        for app in manifest_json_2:
            self.assertIsInstance(app, Webapp)
            for key in Webapp.required_keys:
                self.assertIn(key, app)

    def test_invalid_webapp(self):
        """Tests a webapp with a missing required key"""
        webapps = WebappCollection(self.profile)
        
        self.assertRaises(WebappFormatException, webapps.append, { 'name': 'foo' })

    def test_webapp_collection(self):
        """Tests the methods of the WebappCollection object"""
        webapp_1 = { 'name': 'test_app_1',
                     'description': 'a description',
                     'manifestURL': 'http://example.com/1/manifest.webapp',
                     'appStatus': 1 }

        webapp_2 = { 'name': 'test_app_2',
                     'description': 'another description',
                     'manifestURL': 'http://example.com/2/manifest.webapp',
                     'appStatus': 2 }

        webapp_3 = { 'name': 'test_app_2',
                     'description': 'a third description',
                     'manifestURL': 'http://example.com/3/manifest.webapp',
                     'appStatus': 3 }

        webapps = WebappCollection(self.profile)
        self.assertEqual(len(webapps), 0)

        
        def invalid_index():
            webapps[0]
        self.assertRaises(IndexError, invalid_index)

        
        webapps.append(webapp_1)
        self.assertTrue(len(webapps), 1)
        self.assertIsInstance(webapps[0], Webapp)
        self.assertEqual(len(webapps[0]), len(webapp_1))
        self.assertEqual(len(set(webapps[0].items()) & set(webapp_1.items())), len(webapp_1))

        
        webapps.remove(webapp_1)
        self.assertEqual(len(webapps), 0)

        
        webapps.extend([webapp_1, webapp_2])
        self.assertEqual(len(webapps), 2)
        self.assertTrue(webapp_1 in webapps)
        self.assertTrue(webapp_2 in webapps)
        self.assertNotEquals(webapps[0], webapps[1])

        
        webapps.insert(1, webapp_3)
        self.assertEqual(len(webapps), 3)
        self.assertEqual(webapps[1], webapps[2])
        for app in webapps:
            self.assertIsInstance(app, Webapp)

        
        def invalid_type():
            webapps[2] = 1
        self.assertRaises(WebappFormatException, invalid_type)

    def test_install_webapps(self):
        """Test installing webapps into a profile that has no prior webapps"""
        webapps = WebappCollection(self.profile, apps=self.manifest_path_1)
        self.assertFalse(os.path.exists(self.webapps_dir))

        
        webapps.update_manifests()
        self.assertFalse(os.path.isdir(os.path.join(self.profile, webapps.backup_dir)))
        self.assertTrue(os.path.isfile(self.webapps_json_path))

        webapps_json = webapps.read_json(self.webapps_json_path, description="fake description")
        self.assertEqual(len(webapps_json), 7)
        for app in webapps_json:
            self.assertIsInstance(app, Webapp)

        manifest_json_1 = webapps.read_json(self.manifest_path_1)
        manifest_json_2 = webapps.read_json(self.manifest_path_2)
        self.assertEqual(len(webapps_json), len(manifest_json_1))
        for app in webapps_json:
            self.assertTrue(app in manifest_json_1)

        
        removed_app = manifest_json_1[2]
        webapps.remove(removed_app)
        
        webapps.extend(manifest_json_2)

        
        webapps.update_manifests()
        self.assertFalse(os.path.isdir(os.path.join(self.profile, webapps.backup_dir)))
        self.assertTrue(os.path.isfile(self.webapps_json_path))

        webapps_json = webapps.read_json(self.webapps_json_path, description="a description")
        self.assertEqual(len(webapps_json), 11)

        
        for app in webapps_json:
            self.assertIsInstance(app, Webapp)
            self.assertTrue(os.path.isfile(os.path.join(self.webapps_dir, app['name'], 'manifest.webapp')))
        
        self.assertNotIn(removed_app, webapps_json)
        self.assertFalse(os.path.exists(os.path.join(self.webapps_dir, removed_app['name'])))

        
        webapps.clean()
        self.assertFalse(os.path.isdir(self.webapps_dir))

    def test_install_webapps_preexisting(self):
        """Tests installing webapps when the webapps directory already exists"""
        manifest_json_2 = WebappCollection.read_json(self.manifest_path_2)

        
        os.mkdir(self.webapps_dir)
        shutil.copyfile(self.manifest_path_2, self.webapps_json_path)
        for app in manifest_json_2:
            app_path = os.path.join(self.webapps_dir, app['name'])
            os.mkdir(app_path)
            f = open(os.path.join(app_path, 'manifest.webapp'), 'w')
            f.close()

        webapps = WebappCollection(self.profile, apps=self.manifest_path_1)
        self.assertTrue(os.path.exists(self.webapps_dir))

        
        webapps.update_manifests()
        
        self.assertTrue(os.path.isdir(os.path.join(self.profile, webapps.backup_dir)))

        
        webapps_json = webapps.read_json(self.webapps_json_path, description='a fake description')
        self.assertEqual(len(webapps_json), 12)
        for app in webapps_json:
            self.assertIsInstance(app, Webapp)
            self.assertTrue(os.path.isfile(os.path.join(self.webapps_dir, app['name'], 'manifest.webapp')))

        
        webapps.clean()
        self.assertFalse(os.path.isdir(os.path.join(self.profile, webapps.backup_dir)))

        
        webapps_json = webapps.read_json(self.webapps_json_path)
        for app in webapps_json:
            self.assertIsInstance(app, Webapp)
            self.assertTrue(os.path.isfile(os.path.join(self.webapps_dir, app['name'], 'manifest.webapp')))
        self.assertEqual(webapps_json, manifest_json_2)

if __name__ == '__main__':
    unittest.main()
