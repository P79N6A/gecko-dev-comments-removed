




import sys

def add_libdir_to_path():
    from os.path import dirname, exists, join, realpath
    js_src_dir = dirname(dirname(realpath(sys.argv[0])))
    assert exists(join(js_src_dir,'jsapi.h'))
    sys.path.append(join(js_src_dir, 'lib'))
    sys.path.append(join(js_src_dir, 'tests', 'lib'))

add_libdir_to_path()

import jittests

if __name__ == '__main__':
    jittests.main(sys.argv[1:])
