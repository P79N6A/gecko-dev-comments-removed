





import mozfile
import unittest
import zipfile
from mozversion import get_version


class ApkTest(unittest.TestCase):
    """test getting version information from an android .apk"""

    application_changeset = 'a'*40
    platform_changeset = 'b'*40

    def test_basic(self):
        with mozfile.NamedTemporaryFile() as f:
            with zipfile.ZipFile(f.name, 'w') as z:
                z.writestr('application.ini',
                           """[App]\nSourceStamp=%s\n""" % self.application_changeset)
                z.writestr('platform.ini',
                           """[Build]\nSourceStamp=%s\n""" % self.platform_changeset)
                z.writestr('AndroidManifest.xml', '')
            v = get_version(f.name)
            self.assertEqual(v.get('application_changeset'), self.application_changeset)
            self.assertEqual(v.get('platform_changeset'), self.platform_changeset)

if __name__ == '__main__':
    unittest.main()
