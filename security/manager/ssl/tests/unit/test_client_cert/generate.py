










import tempfile, os, sys, random

libpath = os.path.abspath("../psm_common_py")
sys.path.append(libpath)

import CertUtils

dest_dir = os.getcwd()
db = tempfile.mkdtemp()

serial = random.randint(100, 40000000)
name = "client-cert"
[key, cert] = CertUtils.generate_cert_generic(db, dest_dir, serial, "rsa",
                                              name, "")
CertUtils.generate_pkcs12(db, dest_dir, cert, key, name)


os.remove(dest_dir + "/" + name + ".der")

print ("You now MUST modify ClientAuthServer.cpp to ensure the xpchell debug " +
       "certificate there matches this newly generated one\n")
