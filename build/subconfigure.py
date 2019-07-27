






import argparse
import errno
import os
import re
import subprocess
import sys
import pickle

import mozpack.path as mozpath

class File(object):
    def __init__(self, path):
        self._path = path
        self._content = open(path, 'rb').read()
        stat = os.stat(path)
        self._times = (stat.st_atime, stat.st_mtime)

    @property
    def path(self):
        return self._path

    @property
    def mtime(self):
        return self._times[1]

    def update_time(self):
        '''If the file hasn't changed since the instance was created,
           restore its old modification time.'''
        if not os.path.exists(self._path):
            return
        if open(self._path, 'rb').read() == self._content:
            os.utime(self._path, self._times)



PRECIOUS_VARS = set([
    'build_alias',
    'host_alias',
    'target_alias',
    'CC',
    'CFLAGS',
    'LDFLAGS',
    'LIBS',
    'CPPFLAGS',
    'CPP',
    'CCC',
    'CXXFLAGS',
    'CXX',
    'CCASFLAGS',
    'CCAS',
])








def maybe_clear_cache(data):
    env = dict(data['env'])
    for kind in ('target', 'host', 'build'):
        arg = data[kind]
        if arg is not None:
            env['%s_alias' % kind] = arg
    
    
    for arg in data['args']:
        if arg[:1] != '-' and '=' in arg:
            key, value = arg.split('=', 1)
            env[key] = value

    comment = re.compile(r'^\s+#')
    cache = {}
    with open(data['cache-file']) as f:
        for line in f:
            if not comment.match(line) and '=' in line:
                key, value = line.rstrip(os.linesep).split('=', 1)
                
                if value[:1] == "'":
                    value = value[1:-1].replace("'\\''", "'")
                cache[key] = value
    for precious in PRECIOUS_VARS:
        
        
        if 'ac_cv_env_%s_set' % precious not in cache:
            continue
        is_set = cache.get('ac_cv_env_%s_set' % precious) == 'set'
        value = cache.get('ac_cv_env_%s_value' % precious) if is_set else None
        if value != env.get(precious):
            print 'Removing %s because of %s value change from:' \
                % (data['cache-file'], precious)
            print '  %s' % (value if value is not None else 'undefined')
            print 'to:'
            print '  %s' % env.get(precious, 'undefined')
            os.remove(data['cache-file'])
            return


def split_template(s):
    """Given a "file:template" string, returns "file", "template". If the string
    is of the form "file" (without a template), returns "file", "file.in"."""
    if ':' in s:
        return s.split(':', 1)
    return s, '%s.in' % s


def get_config_files(data):
    config_status = mozpath.join(data['objdir'], 'config.status')
    if not os.path.exists(config_status):
        return [], []

    configure = mozpath.join(data['srcdir'], 'configure')
    config_files = [(config_status, configure)]
    command_files = []

    
    
    config_status_output = subprocess.check_output(
        [data['shell'], '-c', '%s --help' % config_status],
        stderr=subprocess.STDOUT).splitlines()
    state = None
    for line in config_status_output:
        if line.startswith('Configuration') and line.endswith(':'):
            if line.endswith('commands:'):
                state = 'commands'
            else:
                state = 'config'
        elif not line.strip():
            state = None
        elif state:
            for f, t in (split_template(couple) for couple in line.split()):
                f = mozpath.join(data['objdir'], f)
                t = mozpath.join(data['srcdir'], t)
                if state == 'commands':
                    command_files.append(f)
                else:
                    config_files.append((f, t))

    return config_files, command_files


def prepare(data_file, srcdir, objdir, shell, args):
    parser = argparse.ArgumentParser()
    parser.add_argument('--target', type=str)
    parser.add_argument('--host', type=str)
    parser.add_argument('--build', type=str)
    parser.add_argument('--cache-file', type=str)
    
    
    
    
    parser.add_argument('--srcdir', type=str)

    
    
    
    
    
    
    
    
    input = sys.stdin.read()
    if input:
        data = {a: b for [a, b] in eval(input)}
        environ = {a: b for a, b in data['env']}
        environ['PATH'] = os.environ['PATH']
        args = data['args']
    else:
        environ = os.environ

    args, others = parser.parse_known_args(args)

    data = {
        'target': args.target,
        'host': args.host,
        'build': args.build,
        'args': others,
        'shell': shell,
        'srcdir': srcdir,
        'env': environ,
    }

    if args.cache_file:
        data['cache-file'] = mozpath.normpath(mozpath.join(os.getcwd(),
            args.cache_file))
    else:
        data['cache-file'] = mozpath.join(objdir, 'config.cache')

    try:
        os.makedirs(objdir)
    except OSError as e:
        if e.errno != errno.EEXIST:
            raise

    with open(os.path.join(objdir, data_file), 'wb') as f:
        pickle.dump(data, f)


def run(data_file, objdir):
    with open(os.path.join(objdir, data_file), 'rb') as f:
        data = pickle.load(f)

    data['objdir'] = objdir

    cache_file = data['cache-file']
    if os.path.exists(cache_file):
        maybe_clear_cache(data)

    config_files, command_files = get_config_files(data)
    contents = []
    for f, t in config_files:
        contents.append(File(f))

    
    
    
    
    for f in command_files:
        if os.path.isfile(f):
            contents.append(File(f))

    configure = mozpath.join(data['srcdir'], 'configure')
    command = [data['shell'], configure]
    for kind in ('target', 'build', 'host'):
        if data.get(kind) is not None:
            command += ['--%s=%s' % (kind, data[kind])]
    command += data['args']
    command += ['--cache-file=%s' % cache_file]

    print 'configuring in %s' % os.path.relpath(objdir, os.getcwd())
    print 'running %s' % ' '.join(command)
    sys.stdout.flush()
    ret = subprocess.call(command, cwd=objdir, env=data['env'])

    for f in contents:
        
        
        if os.path.basename(f.path) == 'config.status' and \
                os.path.getmtime(configure) > f.mtime:
            continue
        f.update_time()

    return ret


CONFIGURE_DATA = 'configure.pkl'

def main(args):
    if args[0] != '--prepare':
        if len(args) != 1:
            raise Exception('Usage: %s relativeobjdir' % __file__)
        return run(CONFIGURE_DATA, args[0])

    topsrcdir = os.path.abspath(args[1])
    subdir = args[2]
    
    if ':' in subdir:
        srcdir, subdir = subdir.split(':', 1)
    else:
        srcdir = subdir
    srcdir = os.path.join(topsrcdir, srcdir)
    objdir = os.path.abspath(subdir)

    return prepare(CONFIGURE_DATA, srcdir, objdir, args[3], args[4:])


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
