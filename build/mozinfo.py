








































"""
file for interface to transform introspected system information to a format
pallatable to Mozilla

Information:
- os : what operating system ['win', 'mac', 'linux', ...]
- bits : 32 or 64
- processor : processor architecture ['x86', 'x86_64', 'ppc', ...]
- version : operating system version string

For windows, the service pack information is also included
"""





import os
import platform
import re
import sys


_os = os

class unknown(object):
    """marker class for unknown information"""
    def __nonzero__(self):
        return False
    def __str__(self):
        return 'UNKNOWN'
unknown = unknown() 



info = {'os': unknown,
        'processor': unknown,
        'version': unknown,
        'bits': unknown }

(system, node, release, version, machine, processor) = platform.uname()
(bits, linkage) = platform.architecture()

if system in ["Microsoft", "Windows"]:
    info['os'] = 'win'
    
    
    
    if "PROCESSOR_ARCHITEW6432" in os.environ:
        processor = os.environ.get("PROCESSOR_ARCHITEW6432", processor)
    else:
        processor = os.environ.get('PROCESSOR_ARCHITECTURE', processor)
        system = os.environ.get("OS", system).replace('_', ' ')
        service_pack = os.sys.getwindowsversion()[4]
        info['service_pack'] = service_pack
elif system == "Linux":
    (distro, version, codename) = platform.dist()
    version = distro + " " + version
    if not processor:
        processor = machine
    info['os'] = 'linux'
elif system == "Darwin":
    (release, versioninfo, machine) = platform.mac_ver()
    version = "OS X " + release
    info['os'] = 'mac'
elif sys.platform in ('solaris', 'sunos5'):
    info['os'] = 'unix'
    version = sys.platform


if processor in ["i386", "i686"]:
    if bits == "32bit":
        processor = "x86"
    elif bits == "64bit":
        processor = "x86_64"
elif processor == "AMD64":
    bits = "64bit"
    processor = "x86_64"
elif processor == "Power Macintosh":
    processor = "ppc"
bits = re.search('(\d+)bit', bits).group(1)

info.update({'version': version,
             'processor': processor,
             'bits': int(bits),
            })

def update(new_info):
    """update the info"""
    info.update(new_info)
    globals().update(info)
  
update({})

choices = {'os': ['linux', 'win', 'mac', 'unix'],
           'bits': [32, 64],
           'processor': ['x86', 'x86_64', 'ppc']}


__all__ = info.keys()
__all__ += ['info', 'unknown', 'main', 'choices']


def main(args=None):

    
    from optparse import OptionParser
    parser = OptionParser()
    for key in choices:
        parser.add_option('--%s' % key, dest=key,
                          action='store_true', default=False,
                          help="display choices for %s" % key)
    options, args = parser.parse_args()

    
    if args:
        try:
            from json import loads
        except ImportError:
            try:
                from simplejson import loads
            except ImportError:
                def loads(string):
                    """*really* simple json; will not work with unicode"""
                    return eval(string, {'true': True, 'false': False, 'null': None})
        for arg in args:
            if _os.path.exists(arg):
                string = file(arg).read()
            else:
                string = arg
            update(loads(string))

    
    flag = False
    for key, value in options.__dict__.items():
        if value is True:
            print '%s choices: %s' % (key, ' '.join([str(choice)
                                                     for choice in choices[key]]))
            flag = True
    if flag: return

    
    for key, value in info.items():
        print '%s: %s' % (key, value)

if __name__ == '__main__':
    main()
