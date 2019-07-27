







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



print
CertUtils.print_cert_info(cert)
print ('You now MUST update the fingerprint in ClientAuthServer.cpp to match ' +
       'the fingerprint printed above.')


os.remove(dest_dir + "/" + name + ".der")
