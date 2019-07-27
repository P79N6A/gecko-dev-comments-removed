






from __future__ import absolute_import, print_function, unicode_literals

import distutils.sysconfig
import os
import shutil
import subprocess
import sys
import warnings

from distutils.version import LooseVersion



MINIMUM_PYTHON_VERSION = LooseVersion('2.7.3')
MINIMUM_PYTHON_MAJOR = 2


UPGRADE_WINDOWS = '''
Please upgrade to the latest MozillaBuild development environment. See
https://developer.mozilla.org/en-US/docs/Developer_Guide/Build_Instructions/Windows_Prerequisites
'''.lstrip()

UPGRADE_OTHER = '''
Run |mach bootstrap| to ensure your system is up to date.

If you still receive this error, your shell environment is likely detecting
another Python version. Ensure a modern Python can be found in the paths
defined by the $PATH environment variable and try again.
'''.lstrip()


class VirtualenvManager(object):
    """Contains logic for managing virtualenvs for building the tree."""

    def __init__(self, topsrcdir, topobjdir, virtualenv_path, log_handle,
        manifest_path):
        """Create a new manager.

        Each manager is associated with a source directory, a path where you
        want the virtualenv to be created, and a handle to write output to.
        """
        assert os.path.isabs(manifest_path), "manifest_path must be an absolute path: %s" % (manifest_path)
        self.topsrcdir = topsrcdir
        self.topobjdir = topobjdir
        self.virtualenv_root = virtualenv_path
        self.log_handle = log_handle
        self.manifest_path = manifest_path

    @property
    def virtualenv_script_path(self):
        """Path to virtualenv's own populator script."""
        return os.path.join(self.topsrcdir, 'python', 'virtualenv',
            'virtualenv.py')

    @property
    def bin_path(self):
        
        
        
        
        if sys.platform in ('win32', 'cygwin'):
            return os.path.join(self.virtualenv_root, 'Scripts')

        return os.path.join(self.virtualenv_root, 'bin')

    @property
    def python_path(self):
        binary = 'python'
        if sys.platform in ('win32', 'cygwin'):
            binary += '.exe'

        return os.path.join(self.bin_path, binary)

    @property
    def activate_path(self):
        return os.path.join(self.bin_path, 'activate_this.py')

    def up_to_date(self):
        """Returns whether the virtualenv is present and up to date."""

        deps = [self.manifest_path, __file__]

        
        if not os.path.exists(self.virtualenv_root) or \
            not os.path.exists(self.activate_path):

            return False

        
        activate_mtime = os.path.getmtime(self.activate_path)
        dep_mtime = max(os.path.getmtime(p) for p in deps)
        if dep_mtime > activate_mtime:
            return False

        
        submanifests = [i[1] for i in self.packages()
                        if i[0] == 'packages.txt']
        for submanifest in submanifests:
            submanifest = os.path.join(self.topsrcdir, submanifest)
            submanager = VirtualenvManager(self.topsrcdir,
                                           self.topobjdir,
                                           self.virtualenv_root,
                                           self.log_handle,
                                           submanifest)
            if not submanager.up_to_date():
                return False

        return True

    def ensure(self):
        """Ensure the virtualenv is present and up to date.

        If the virtualenv is up to date, this does nothing. Otherwise, it
        creates and populates the virtualenv as necessary.

        This should be the main API used from this class as it is the
        highest-level.
        """
        if self.up_to_date():
            return self.virtualenv_root
        return self.build()

    def create(self):
        """Create a new, empty virtualenv.

        Receives the path to virtualenv's virtualenv.py script (which will be
        called out to), the path to create the virtualenv in, and a handle to
        write output to.
        """
        env = dict(os.environ)
        env.pop('PYTHONDONTWRITEBYTECODE', None)

        args = [sys.executable, self.virtualenv_script_path,
            self.virtualenv_root]

        result = subprocess.call(args, stdout=self.log_handle,
            stderr=subprocess.STDOUT, env=env)

        if result:
            raise Exception('Error creating virtualenv.')

        return self.virtualenv_root

    def packages(self):
        with file(self.manifest_path, 'rU') as fh:
            packages = [line.rstrip().split(':')
                        for line in fh]
        return packages

    def populate(self):
        """Populate the virtualenv.

        The manifest file consists of colon-delimited fields. The first field
        specifies the action. The remaining fields are arguments to that
        action. The following actions are supported:

        setup.py -- Invoke setup.py for a package. Expects the arguments:
            1. relative path directory containing setup.py.
            2. argument(s) to setup.py. e.g. "develop". Each program argument
               is delimited by a colon. Arguments with colons are not yet
               supported.

        filename.pth -- Adds the path given as argument to filename.pth under
            the virtualenv site packages directory.

        optional -- This denotes the action as optional. The requested action
            is attempted. If it fails, we issue a warning and go on. The
            initial "optional" field is stripped then the remaining line is
            processed like normal. e.g.
            "optional:setup.py:python/foo:built_ext:-i"

        copy -- Copies the given file in the virtualenv site packages
            directory.

        packages.txt -- Denotes that the specified path is a child manifest. It
            will be read and processed as if its contents were concatenated
            into the manifest being read.

        objdir -- Denotes a relative path in the object directory to add to the
            search path. e.g. "objdir:build" will add $topobjdir/build to the
            search path.

        Note that the Python interpreter running this function should be the
        one from the virtualenv. If it is the system Python or if the
        environment is not configured properly, packages could be installed
        into the wrong place. This is how virtualenv's work.
        """

        packages = self.packages()

        def handle_package(package):
            python_lib = distutils.sysconfig.get_python_lib()
            if package[0] == 'setup.py':
                assert len(package) >= 2

                self.call_setup(os.path.join(self.topsrcdir, package[1]),
                    package[2:])

                return True

            if package[0] == 'copy':
                assert len(package) == 2

                src = os.path.join(self.topsrcdir, package[1])
                dst = os.path.join(python_lib, os.path.basename(package[1]))

                shutil.copy(src, dst)

                return True

            if package[0] == 'packages.txt':
                assert len(package) == 2

                src = os.path.join(self.topsrcdir, package[1])
                assert os.path.isfile(src), "'%s' does not exist" % src
                submanager = VirtualenvManager(self.topsrcdir,
                                               self.topobjdir,
                                               self.virtualenv_root,
                                               self.log_handle,
                                               src)
                submanager.populate()

                return True

            if package[0].endswith('.pth'):
                assert len(package) == 2

                path = os.path.join(self.topsrcdir, package[1])

                with open(os.path.join(python_lib, package[0]), 'a') as f:
                    
                    
                    
                    
                    try:
                        f.write("%s\n" % os.path.relpath(path, python_lib))
                    except ValueError:
                        
                        f.write("%s\n" % os.path.join(python_lib, path))

                return True

            if package[0] == 'optional':
                try:
                    handle_package(package[1:])
                    return True
                except:
                    print('Error processing command. Ignoring', \
                        'because optional. (%s)' % ':'.join(package),
                        file=self.log_handle)
                    return False

            if package[0] == 'objdir':
                assert len(package) == 2
                path = os.path.join(self.topobjdir, package[1])

                with open(os.path.join(python_lib, 'objdir.pth'), 'a') as f:
                    f.write('%s\n' % path)

                return True

            raise Exception('Unknown action: %s' % package[0])

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        IGNORE_ENV_VARIABLES = ('CC', 'CXX', 'CFLAGS', 'CXXFLAGS', 'LDFLAGS',
            'PYTHONDONTWRITEBYTECODE')

        try:
            old_target = os.environ.get('MACOSX_DEPLOYMENT_TARGET', None)
            sysconfig_target = \
                distutils.sysconfig.get_config_var('MACOSX_DEPLOYMENT_TARGET')

            if sysconfig_target is not None:
                os.environ['MACOSX_DEPLOYMENT_TARGET'] = sysconfig_target

            old_env_variables = {}
            for k in IGNORE_ENV_VARIABLES:
                if k not in os.environ:
                    continue

                old_env_variables[k] = os.environ[k]
                del os.environ[k]

            
            
            
            
            
            
            
            
            
            
            if sys.platform in ('win32', 'cygwin') and \
                'VS90COMNTOOLS' not in os.environ:

                warnings.warn('Hacking environment to allow binary Python '
                    'extensions to build. You can make this warning go away '
                    'by installing Visual Studio 2008. You can download the '
                    'Express Edition installer from '
                    'http://go.microsoft.com/?linkid=7729279')

                
                
                for ver in ('100', '110', '120'):
                    var = 'VS%sCOMNTOOLS' % ver
                    if var in os.environ:
                        os.environ['VS90COMNTOOLS'] = os.environ[var]
                        break

            for package in packages:
                handle_package(package)
        finally:
            os.environ.pop('MACOSX_DEPLOYMENT_TARGET', None)

            if old_target is not None:
                os.environ['MACOSX_DEPLOYMENT_TARGET'] = old_target

            os.environ.update(old_env_variables)

    def call_setup(self, directory, arguments):
        """Calls setup.py in a directory."""
        setup = os.path.join(directory, 'setup.py')

        program = [self.python_path, setup]
        program.extend(arguments)

        
        
        
        

        try:
            output = subprocess.check_output(program, cwd=directory, stderr=subprocess.STDOUT)
            print(output)
        except subprocess.CalledProcessError as e:
            if 'Python.h: No such file or directory' in e.output:
                print('WARNING: Python.h not found. Install Python development headers.')
            else:
                print(e.output)

            raise Exception('Error installing package: %s' % directory)

    def build(self):
        """Build a virtualenv per tree conventions.

        This returns the path of the created virtualenv.
        """

        self.create()

        
        

        args = [self.python_path, __file__, 'populate', self.topsrcdir,
            self.topobjdir, self.virtualenv_root, self.manifest_path]

        result = subprocess.call(args, stdout=self.log_handle,
            stderr=subprocess.STDOUT, cwd=self.topsrcdir)

        if result != 0:
            raise Exception('Error populating virtualenv.')

        os.utime(self.activate_path, None)

        return self.virtualenv_root

    def activate(self):
        """Activate the virtualenv in this Python context.

        If you run a random Python script and wish to "activate" the
        virtualenv, you can simply instantiate an instance of this class
        and call .ensure() and .activate() to make the virtualenv active.
        """

        execfile(self.activate_path, dict(__file__=self.activate_path))
        if isinstance(os.environ['PATH'], unicode):
            os.environ['PATH'] = os.environ['PATH'].encode('utf-8')

    def install_pip_package(self, package):
        """Install a package via pip.

        The supplied package is specified using a pip requirement specifier.
        e.g. 'foo' or 'foo==1.0'.

        If the package is already installed, this is a no-op.
        """
        from pip.req import InstallRequirement

        req = InstallRequirement.from_line(package)
        if req.check_if_exists():
            return

        args = [
            'install',
            '--use-wheel',
            package,
        ]

        return self._run_pip(args)

    def _run_pip(self, args):
        
        
        
        
        
        
        
        subprocess.check_call([os.path.join(self.bin_path, 'pip')] + args,
            stderr=subprocess.STDOUT)


def verify_python_version(log_handle):
    """Ensure the current version of Python is sufficient."""
    major, minor, micro = sys.version_info[:3]

    our = LooseVersion('%d.%d.%d' % (major, minor, micro))

    if major != MINIMUM_PYTHON_MAJOR or our < MINIMUM_PYTHON_VERSION:
        log_handle.write('Python %s or greater (but not Python 3) is '
            'required to build. ' % MINIMUM_PYTHON_VERSION)
        log_handle.write('You are running Python %s.\n' % our)

        if os.name in ('nt', 'ce'):
            log_handle.write(UPGRADE_WINDOWS)
        else:
            log_handle.write(UPGRADE_OTHER)

        sys.exit(1)


if __name__ == '__main__':
    if len(sys.argv) < 5:
        print('Usage: populate_virtualenv.py /path/to/topsrcdir /path/to/topobjdir /path/to/virtualenv /path/to/virtualenv_manifest')
        sys.exit(1)

    verify_python_version(sys.stdout)

    topsrcdir, topobjdir, virtualenv_path, manifest_path = sys.argv[1:5]
    populate = False

    
    if sys.argv[1] == 'populate':
        populate = True
        topsrcdir, topobjdir, virtualenv_path, manifest_path = sys.argv[2:]

    manager = VirtualenvManager(topsrcdir, topobjdir, virtualenv_path,
        sys.stdout, manifest_path)

    if populate:
        manager.populate()
    else:
        manager.ensure()

