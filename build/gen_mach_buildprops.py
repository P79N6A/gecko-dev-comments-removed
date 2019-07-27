





import sys
import os
import sha
import json
import re
import errno
from argparse import ArgumentParser

def getFileHashAndSize(filename):
    sha1Hash = 'UNKNOWN'
    size = 'UNKNOWN'

    try:
        
        
        f = open(filename, "rb")
        shaObj = sha.new(f.read())
        sha1Hash = shaObj.hexdigest()

        size = os.path.getsize(filename)
    except:
        pass

    return (sha1Hash, size)

def getMarProperties(filename):
    if not os.path.exists(filename):
        return {}
    (complete_mar_hash, complete_mar_size) = getFileHashAndSize(filename)
    return {
        'completeMarFilename': os.path.basename(filename),
        'completeMarSize': complete_mar_size,
        'completeMarHash': complete_mar_hash,
    }

def getUrlProperties(filename):
    
    
    property_conditions = [
        
        ('symbolsUrl', lambda m: m.endswith('crashreporter-symbols.zip') or
                       m.endswith('crashreporter-symbols-full.zip')),
        ('testsUrl', lambda m: m.endswith(('tests.tar.bz2', 'tests.zip'))),
        ('unsignedApkUrl', lambda m: m.endswith('apk') and
                           'unsigned-unaligned' in m),
        ('robocopApkUrl', lambda m: m.endswith('apk') and 'robocop' in m),
        ('jsshellUrl', lambda m: 'jsshell-' in m and m.endswith('.zip')),
        ('completeMarUrl', lambda m: m.endswith('.complete.mar')),
        
        ('packageUrl', lambda m: True),
    ]
    url_re = re.compile(r'''^(https?://.*?\.(?:tar\.bz2|dmg|zip|apk|rpm|mar|tar\.gz))$''')
    properties = {}

    try:
        with open(filename) as f:
            for line in f:
                m = url_re.match(line)
                if m:
                    m = m.group(1)
                    for prop, condition in property_conditions:
                        if condition(m):
                            properties.update({prop: m})
                            break
    except IOError as e:
        if e.errno != errno.ENOENT:
            raise
        properties = {prop: 'UNKNOWN' for prop, condition in property_conditions}
    return properties

if __name__ == '__main__':
    parser = ArgumentParser(description='Generate mach_build_properties.json for automation builds.')
    parser.add_argument("--complete-mar-file", required=True,
                        action="store", dest="complete_mar_file",
                        help="Path to the complete MAR file, relative to the objdir.")
    parser.add_argument("--upload-output", required=True,
                        action="store", dest="upload_output",
                        help="Path to the text output of 'make upload'")
    args = parser.parse_args()

    json_data = getMarProperties(args.complete_mar_file)
    json_data.update(getUrlProperties(args.upload_output))

    with open('mach_build_properties.json', 'w') as outfile:
        json.dump(json_data, outfile, indent=4)
