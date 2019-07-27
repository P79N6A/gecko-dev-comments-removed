



from __future__ import absolute_import, print_function

import argparse
import sys


def android_version_code(buildid, cpu_arch=None, min_sdk=0, max_sdk=0):
    base = int(str(buildid)[:10])
    
    if not cpu_arch or cpu_arch in ['armeabi', 'armeabi-v7a']:
        
        
        return base + min_sdk + 0
    elif cpu_arch in ['x86']:
        
        
        
        
        return base + min_sdk + 3
    else:
        raise ValueError("Don't know how to compute android:versionCode "
                         "for CPU arch " + cpu_arch)


def main(argv):
    parser = argparse.ArgumentParser('Generate an android:versionCode',
                                     add_help=False)
    parser.add_argument('--verbose', action='store_true',
                        default=False,
                        help='Be verbose')
    parser.add_argument('--with-android-cpu-arch', dest='cpu_arch',
                        choices=['armeabi', 'armeabi-v7a', 'mips', 'x86'],
                        help='The target CPU architecture')
    parser.add_argument('--with-android-min-sdk-version', dest='min_sdk',
                        type=int, default=0,
                        help='The minimum target SDK')
    parser.add_argument('--with-android-max-sdk-version', dest='max_sdk',
                        type=int, default=0,
                        help='The maximum target SDK')
    parser.add_argument('buildid', type=int,
                        help='The input build ID')

    args = parser.parse_args(argv)
    code = android_version_code(args.buildid,
        cpu_arch=args.cpu_arch,
        min_sdk=args.min_sdk,
        max_sdk=args.max_sdk)
    print(code)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
