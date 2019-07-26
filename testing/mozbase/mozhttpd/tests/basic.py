

import mozhttpd
import mozfile
import os
import tempfile
import unittest


class TestBasic(unittest.TestCase):
    """ Test basic Mozhttpd capabilites """

    def test_basic(self):
        """ Test mozhttpd can serve files """

        tempdir = tempfile.mkdtemp()

        
        sizes = {'small': [128], 'large': [16384]}

        for k in sizes.keys():
            
            sizes[k].append(os.urandom(sizes[k][0]))

            
            fpath = os.path.join(tempdir, k)
            sizes[k].append(fpath)

            
            with open(fpath, 'wb') as f:
                f.write(sizes[k][1])

        server = mozhttpd.MozHttpd(docroot=tempdir)
        server.start()
        server_url = server.get_url()

        
        for k in sizes.keys():
            retrieved_content = mozfile.load(server_url + k).read()
            self.assertEqual(retrieved_content, sizes[k][1])

        
        mozfile.rmtree(tempdir)

if __name__ == '__main__':
    unittest.main()
