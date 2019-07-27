'''Tests block operations.'''
from rsa._compat import b

try:
    from StringIO import StringIO as BytesIO
except ImportError:
    from io import BytesIO
import unittest2

import rsa
from rsa import bigfile, varblock, pkcs1

class BigfileTest(unittest2.TestCase):

    def test_encrypt_decrypt_bigfile(self):

        
        pub_key, priv_key = rsa.newkeys((6 + 11) * 8)

        
        message = b('123456Sybren')
        infile = BytesIO(message)
        outfile = BytesIO()

        bigfile.encrypt_bigfile(infile, outfile, pub_key)

        
        crypto = outfile.getvalue()

        cryptfile = BytesIO(crypto)
        clearfile = BytesIO()

        bigfile.decrypt_bigfile(cryptfile, clearfile, priv_key)
        self.assertEquals(clearfile.getvalue(), message)
        
        
        
        cryptfile.seek(0)
        varblocks = list(varblock.yield_varblocks(cryptfile))
        self.assertEqual(2, len(varblocks))


    def test_sign_verify_bigfile(self):

        
        pub_key, priv_key = rsa.newkeys((34 + 11) * 8)

        
        msgfile = BytesIO(b('123456Sybren'))
        signature = pkcs1.sign(msgfile, priv_key, 'MD5')

        
        msgfile.seek(0)
        self.assertTrue(pkcs1.verify(msgfile, signature, pub_key))

        
        msgfile = BytesIO(b('123456sybren'))
        self.assertRaises(pkcs1.VerificationError,
            pkcs1.verify, msgfile, signature, pub_key)

