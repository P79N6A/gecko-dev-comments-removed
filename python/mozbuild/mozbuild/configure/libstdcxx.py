














from __future__ import absolute_import

import os
import subprocess
import re

re_for_ld = re.compile('.*\((.*)\).*')

def parse_readelf_line(x):
    """Return the version from a readelf line that looks like:
    0x00ec: Rev: 1  Flags: none  Index: 8  Cnt: 2  Name: GLIBCXX_3.4.6
    """
    return x.split(':')[-1].split('_')[-1].strip()

def parse_ld_line(x):
    """Parse a line from the output of ld -t. The output of gold is just
    the full path, gnu ld prints "-lstdc++ (path)".
    """
    t = re_for_ld.match(x)
    if t:
        return t.groups()[0].strip()
    return x.strip()

def split_ver(v):
    """Covert the string '1.2.3' into the list [1,2,3]
    """
    return [int(x) for x in v.split('.')]

def cmp_ver(a, b):
    """Compare versions in the form 'a.b.c'
    """
    for (i, j) in zip(split_ver(a), split_ver(b)):
        if i != j:
            return i - j
    return 0

def encode_ver(v):
    """Encode the version as a single number.
    """
    t = split_ver(v)
    return t[0] << 16 | t[1] << 8 | t[2]

def find_version(e):
    """Given the value of environment variable CXX or HOST_CXX, find the
    version of the libstdc++ it uses.
    """
    args = e.split()
    args +=  ['-shared', '-Wl,-t']
    p = subprocess.Popen(args, stderr=subprocess.STDOUT, stdout=subprocess.PIPE)
    candidates = [x for x in p.stdout if 'libstdc++.so' in x]
    if not candidates:
        return ''
    assert len(candidates) == 1
    libstdcxx = parse_ld_line(candidates[-1])

    p = subprocess.Popen(['readelf', '-V', libstdcxx], stdout=subprocess.PIPE)
    versions = [parse_readelf_line(x)
                for x in p.stdout.readlines() if 'Name: GLIBCXX' in x]
    last_version = sorted(versions, cmp = cmp_ver)[-1]
    return encode_ver(last_version)

if __name__ == '__main__':
    cxx_env = os.environ['CXX']
    print 'MOZ_LIBSTDCXX_TARGET_VERSION=%s' % find_version(cxx_env)
    host_cxx_env = os.environ.get('HOST_CXX', cxx_env)
    print 'MOZ_LIBSTDCXX_HOST_VERSION=%s' % find_version(host_cxx_env)
