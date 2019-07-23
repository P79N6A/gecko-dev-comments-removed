
























import base64
import os.path
import re
import sys
import textwrap

objects = []


in_data, in_multiline, in_obj = False, False, False
field, type, value, obj = None, None, None, dict()
for line in sys.stdin: 
    
    if not in_data:
        if line.startswith('BEGINDATA'):
            in_data = True
        continue
    
    if line.startswith('#'):
        continue
    
    if in_obj and len(line.strip()) == 0:
        objects.append(obj)
        obj = dict()
        in_obj = False
        continue
    if len(line.strip()) == 0:
        continue
    if in_multiline:
        if not line.startswith('END'):
            if type == 'MULTILINE_OCTAL':
                line = line.strip()
                for i in re.finditer(r'\\([0-3][0-7][0-7])', line):
                    value += chr(int(i.group(1), 8))
            else:
                value += line
            continue
        obj[field] = value
        in_multiline = False
        continue
    if line.startswith('CKA_CLASS'):
        in_obj = True
    line_parts = line.strip().split(' ', 2)
    if len(line_parts) > 2:
        field, type = line_parts[0:2]
        value = ' '.join(line_parts[2:])
    elif len(line_parts) == 2:
        field, type = line_parts
        value = None
    else:
        raise NotImplementedError, 'line_parts < 2 not supported.'
    if type == 'MULTILINE_OCTAL':
        in_multiline = True
        value = ""
        continue
    obj[field] = value
if len(obj.items()) > 0:
    objects.append(obj)


trust = dict()
for obj in objects:
    if obj['CKA_CLASS'] != 'CKO_NETSCAPE_TRUST':
        continue
    
    
    
    
    
    
    
    
    if obj['CKA_LABEL'] == '"ACEDICOM Root"':
        continue
    
    if obj['CKA_TRUST_SERVER_AUTH'] == 'CKT_NETSCAPE_TRUSTED_DELEGATOR':
        trust[obj['CKA_LABEL']] = True

for obj in objects:
    if obj['CKA_CLASS'] == 'CKO_CERTIFICATE':
        if not obj['CKA_LABEL'] in trust or not trust[obj['CKA_LABEL']]:
            continue
        sys.stdout.write("-----BEGIN CERTIFICATE-----\n")
        sys.stdout.write("\n".join(textwrap.wrap(base64.b64encode(obj['CKA_VALUE']), 64)))
        sys.stdout.write("\n-----END CERTIFICATE-----\n\n")

