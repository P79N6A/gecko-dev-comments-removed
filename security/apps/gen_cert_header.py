



import binascii

def _file_byte_generator(filename):
  with open(filename, "rb") as f:
    contents = f.read()

    
    
    
    if not contents:
      return ['\0']

    return contents

def _create_header(array_name, cert_bytes):
  hexified = ["0x" + binascii.hexlify(byte) for byte in cert_bytes]
  substs = { 'array_name': array_name, 'bytes': ', '.join(hexified) }
  return "const uint8_t %(array_name)s[] = {\n%(bytes)s\n};\n" % substs






array_names = [
  'marketplaceProdPublicRoot',
  'marketplaceProdReviewersRoot',
  'marketplaceDevPublicRoot',
  'marketplaceDevReviewersRoot',
  'marketplaceStageRoot',
  'trustedAppPublicRoot',
  'trustedAppTestRoot',
  'xpcshellRoot',
]

for n in array_names:
  
  globals()[n] = lambda header, cert_filename, name=n: header.write(_create_header(name, _file_byte_generator(cert_filename)))
