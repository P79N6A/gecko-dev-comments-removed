

import tempfile, os, sys
import random

libpath = os.path.abspath('../psm_common_py')
sys.path.append(libpath)

import CertUtils

dest_dir = os.getcwd()
db = tempfile.mkdtemp()

CA_basic_constraints = "basicConstraints = critical, CA:TRUE\n"
CA_min_ku = "keyUsage = critical, digitalSignature, keyCertSign, cRLSign\n"
subject_key_ident = "subjectKeyIdentifier = hash\n"

cert_name = 'evroot'
ext_text = CA_basic_constraints + CA_min_ku + subject_key_ident
subject_string = ('/C=US/ST=CA/L=Mountain View' +
                  '/O=Mozilla - EV debug test CA/OU=Security Engineering' +
                  '/CN=XPCShell EV Testing (untrustworthy) CA')



[ca_key, ca_cert] = CertUtils.generate_cert_generic(
    dest_dir,
    dest_dir,
    random.randint(100, 40000000),
    'rsa',
    cert_name,
    ext_text,
    subject_string = subject_string)

CertUtils.generate_pkcs12(db, dest_dir, ca_cert, ca_key, cert_name)



print
CertUtils.print_cert_info(ca_cert)
print ('You now MUST update the compiled test EV root information to match ' +
       'the EV root information printed above. In addition, certs that chain ' +
       'up to this root in other folders will also need to be regenerated.' )
