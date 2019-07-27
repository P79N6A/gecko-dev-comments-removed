



__all__ = ['ManifestParser', 'TestManifest', 'convert']

from StringIO import StringIO
import json
import fnmatch
import os
import shutil
import sys

from .ini import read_ini
from .filters import (
    DEFAULT_FILTERS,
    enabled,
    exists as _exists,
    filterlist,
)

relpath = os.path.relpath
string = (basestring,)




def normalize_path(path):
    """normalize a relative path"""
    if sys.platform.startswith('win'):
        return path.replace('/', os.path.sep)
    return path

def denormalize_path(path):
    """denormalize a relative path"""
    if sys.platform.startswith('win'):
        return path.replace(os.path.sep, '/')
    return path




class ManifestParser(object):
    """read .ini manifests"""

    def __init__(self, manifests=(), defaults=None, strict=True):
        self._defaults = defaults or {}
        self._ancestor_defaults = {}
        self.tests = []
        self.manifest_defaults = {}
        self.strict = strict
        self.rootdir = None
        self.relativeRoot = None
        if manifests:
            self.read(*manifests)

    def getRelativeRoot(self, root):
        return root

    

    def _read(self, root, filename, defaults, defaults_only=False, parentmanifest=None):
        """
        Internal recursive method for reading and parsing manifests.
        Stores all found tests in self.tests
        :param root: The base path
        :param filename: File object or string path for the base manifest file
        :param defaults: Options that apply to all items
        :param defaults_only: If True will only gather options, not include
                              tests. Used for upstream parent includes
                              (default False)
        :param parentmanifest: Filename of the parent manifest (default None)
        """
        def read_file(type):
            include_file = section.split(type, 1)[-1]
            include_file = normalize_path(include_file)
            if not os.path.isabs(include_file):
                include_file = os.path.join(self.getRelativeRoot(here), include_file)
            if not os.path.exists(include_file):
                message = "Included file '%s' does not exist" % include_file
                if self.strict:
                    raise IOError(message)
                else:
                    sys.stderr.write("%s\n" % message)
                    return
            return include_file

        
        if isinstance(filename, string):
            filename = os.path.abspath(filename)
            fp = open(filename)
            here = os.path.dirname(filename)
        else:
            fp = filename
            filename = here = None
        defaults['here'] = here

        
        
        if self.rootdir is None:
            rootdir = ""
        else:
            assert os.path.isabs(self.rootdir)
            rootdir = self.rootdir + os.path.sep

        
        sections = read_ini(fp=fp, variables=defaults, strict=self.strict)
        self.manifest_defaults[filename] = defaults

        parent_section_found = False

        
        for section, data in sections:
            subsuite = ''
            if 'subsuite' in data:
                subsuite = data['subsuite']

            
            
            if defaults_only and not section.startswith('parent:'):
                continue

            
            if section.startswith('parent:'):
                parent_section_found = True

                include_file = read_file('parent:')
                if include_file:
                    self._read(root, include_file, {}, True)
                continue

            
            
            
            if section.startswith('include:'):
                include_file = read_file('include:')
                if include_file:
                    include_defaults = data.copy()
                    self._read(root, include_file, include_defaults, parentmanifest=filename)
                continue

            
            
            data = dict(self._ancestor_defaults.items() + data.items())

            test = data
            test['name'] = section

            
            test['manifest'] = filename

            
            path = test.get('path', section)
            _relpath = path
            if '://' not in path: 
                path = normalize_path(path)
                if here and not os.path.isabs(path):
                    path = os.path.normpath(os.path.join(here, path))

                
                
                
                
                
                
                
                
                
                
                
                
                if path.startswith(rootdir):
                    _relpath = path[len(rootdir):]
                else:
                    _relpath = relpath(path, rootdir)

            test['subsuite'] = subsuite
            test['path'] = path
            test['relpath'] = _relpath

            if parentmanifest is not None:
                
                
                
                
                test['ancestor-manifest'] = parentmanifest

            
            self.tests.append(test)

        
        
        if defaults_only and not parent_section_found:
            sections = read_ini(fp=fp, variables=defaults, defaults_only=True,
                                strict=self.strict)
            (section, self._ancestor_defaults) = sections[0]

    def read(self, *filenames, **defaults):
        """
        read and add manifests from file paths or file-like objects

        filenames -- file paths or file-like objects to read as manifests
        defaults -- default variables
        """

        
        missing = [filename for filename in filenames
                   if isinstance(filename, string) and not os.path.exists(filename) ]
        if missing:
            raise IOError('Missing files: %s' % ', '.join(missing))

        
        _defaults = defaults.copy() or self._defaults.copy()
        _defaults.setdefault('here', None)

        
        for filename in filenames:
            
            defaults = _defaults.copy()
            here = None
            if isinstance(filename, string):
                here = os.path.dirname(os.path.abspath(filename))
                defaults['here'] = here 

            if self.rootdir is None:
                
                
                self.rootdir = here

            self._read(here, filename, defaults)


    

    def query(self, *checks, **kw):
        """
        general query function for tests
        - checks : callable conditions to test if the test fulfills the query
        """
        tests = kw.get('tests', None)
        if tests is None:
            tests = self.tests
        retval = []
        for test in tests:
            for check in checks:
                if not check(test):
                    break
            else:
                retval.append(test)
        return retval

    def get(self, _key=None, inverse=False, tags=None, tests=None, **kwargs):
        
        

        
        

        
        if tags:
            tags = set(tags)
        else:
            tags = set()

        
        if inverse:
            has_tags = lambda test: not tags.intersection(test.keys())
            def dict_query(test):
                for key, value in kwargs.items():
                    if test.get(key) == value:
                        return False
                return True
        else:
            has_tags = lambda test: tags.issubset(test.keys())
            def dict_query(test):
                for key, value in kwargs.items():
                    if test.get(key) != value:
                        return False
                return True

        
        tests = self.query(has_tags, dict_query, tests=tests)

        
        
        if _key:
            return [test[_key] for test in tests]

        
        return tests

    def manifests(self, tests=None):
        """
        return manifests in order in which they appear in the tests
        """
        if tests is None:
            
            return self.manifest_defaults.keys()

        manifests = []
        for test in tests:
            manifest = test.get('manifest')
            if not manifest:
                continue
            if manifest not in manifests:
                manifests.append(manifest)
        return manifests

    def paths(self):
        return [i['path'] for i in self.tests]


    

    def missing(self, tests=None):
        """
        return list of tests that do not exist on the filesystem
        """
        if tests is None:
            tests = self.tests
        existing = list(_exists(tests, {}))
        return [t for t in tests if t not in existing]

    def check_missing(self, tests=None):
        missing = self.missing(tests=tests)
        if missing:
            missing_paths = [test['path'] for test in missing]
            if self.strict:
                raise IOError("Strict mode enabled, test paths must exist. "
                              "The following test(s) are missing: %s" %
                              json.dumps(missing_paths, indent=2))
            print >> sys.stderr, "Warning: The following test(s) are missing: %s" % \
                                  json.dumps(missing_paths, indent=2)
        return missing

    def verifyDirectory(self, directories, pattern=None, extensions=None):
        """
        checks what is on the filesystem vs what is in a manifest
        returns a 2-tuple of sets:
        (missing_from_filesystem, missing_from_manifest)
        """

        files = set([])
        if isinstance(directories, basestring):
            directories = [directories]

        
        for directory in directories:
            for dirpath, dirnames, filenames in os.walk(directory, topdown=True):

                
                if pattern:
                    filenames = fnmatch.filter(filenames, pattern)

                
                if extensions:
                    filenames = [filename for filename in filenames
                                 if os.path.splitext(filename)[-1] in extensions]

                files.update([os.path.join(dirpath, filename) for filename in filenames])

        paths = set(self.paths())
        missing_from_filesystem = paths.difference(files)
        missing_from_manifest = files.difference(paths)
        return (missing_from_filesystem, missing_from_manifest)


    

    def write(self, fp=sys.stdout, rootdir=None,
              global_tags=None, global_kwargs=None,
              local_tags=None, local_kwargs=None):
        """
        write a manifest given a query
        global and local options will be munged to do the query
        globals will be written to the top of the file
        locals (if given) will be written per test
        """

        
        close = False
        if isinstance(fp, string):
            fp = file(fp, 'w')
            close = True

        
        if rootdir is None:
            rootdir = self.rootdir

        
        global_tags = global_tags or set()
        local_tags = local_tags or set()
        global_kwargs = global_kwargs or {}
        local_kwargs = local_kwargs or {}

        
        tags = set([])
        tags.update(global_tags)
        tags.update(local_tags)
        kwargs = {}
        kwargs.update(global_kwargs)
        kwargs.update(local_kwargs)

        
        tests = self.get(tags=tags, **kwargs)

        
        if global_tags or global_kwargs:
            print >> fp, '[DEFAULT]'
            for tag in global_tags:
                print >> fp, '%s =' % tag
            for key, value in global_kwargs.items():
                print >> fp, '%s = %s' % (key, value)
            print >> fp

        for test in tests:
            test = test.copy() 

            path = test['name']
            if not os.path.isabs(path):
                path = test['path']
                if self.rootdir:
                    path = relpath(test['path'], self.rootdir)
                path = denormalize_path(path)
            print >> fp, '[%s]' % path

            
            reserved = ['path', 'name', 'here', 'manifest', 'relpath', 'ancestor-manifest']
            for key in sorted(test.keys()):
                if key in reserved:
                    continue
                if key in global_kwargs:
                    continue
                if key in global_tags and not test[key]:
                    continue
                print >> fp, '%s = %s' % (key, test[key])
            print >> fp

        if close:
            
            fp.close()

    def __str__(self):
        fp = StringIO()
        self.write(fp=fp)
        value = fp.getvalue()
        return value

    def copy(self, directory, rootdir=None, *tags, **kwargs):
        """
        copy the manifests and associated tests
        - directory : directory to copy to
        - rootdir : root directory to copy to (if not given from manifests)
        - tags : keywords the tests must have
        - kwargs : key, values the tests must match
        """
        
        
        
        

        
        if not os.path.exists(directory):
            os.path.makedirs(directory)
        else:
            
            assert os.path.isdir(directory)

        
        tests = self.get(tags=tags, **kwargs)
        if not tests:
            return 

        
        if rootdir is None:
            rootdir = self.rootdir

        
        manifests = [relpath(manifest, rootdir) for manifest in self.manifests()]
        for manifest in manifests:
            destination = os.path.join(directory, manifest)
            dirname = os.path.dirname(destination)
            if not os.path.exists(dirname):
                os.makedirs(dirname)
            else:
                
                assert os.path.isdir(dirname)
            shutil.copy(os.path.join(rootdir, manifest), destination)

        missing = self.check_missing(tests)
        tests = [test for test in tests if test not in missing]
        for test in tests:
            if os.path.isabs(test['name']):
                continue
            source = test['path']
            destination = os.path.join(directory, relpath(test['path'], rootdir))
            shutil.copy(source, destination)
            

    def update(self, from_dir, rootdir=None, *tags, **kwargs):
        """
        update the tests as listed in a manifest from a directory
        - from_dir : directory where the tests live
        - rootdir : root directory to copy to (if not given from manifests)
        - tags : keys the tests must have
        - kwargs : key, values the tests must match
        """

        
        tests = self.get(tags=tags, **kwargs)

        
        if not rootdir:
            rootdir = self.rootdir

        
        for test in tests:
            if not os.path.isabs(test['name']):
                _relpath = relpath(test['path'], rootdir)
                source = os.path.join(from_dir, _relpath)
                if not os.path.exists(source):
                    message = "Missing test: '%s' does not exist!"
                    if self.strict:
                        raise IOError(message)
                    print >> sys.stderr, message + " Skipping."
                    continue
                destination = os.path.join(rootdir, _relpath)
                shutil.copy(source, destination)

    

    @classmethod
    def _walk_directories(cls, directories, callback, pattern=None, ignore=()):
        """
        internal function to import directories
        """

        if isinstance(pattern, basestring):
            patterns = [pattern]
        else:
            patterns = pattern
        ignore = set(ignore)

        if not patterns:
            accept_filename = lambda filename: True
        else:
            def accept_filename(filename):
                for pattern in patterns:
                    if fnmatch.fnmatch(filename, pattern):
                        return True

        if not ignore:
            accept_dirname = lambda dirname: True
        else:
            accept_dirname = lambda dirname: dirname not in ignore

        rootdirectories = directories[:]
        seen_directories = set()
        for rootdirectory in rootdirectories:
            
            directories = [os.path.realpath(rootdirectory)]
            while directories:
                directory = directories.pop(0)
                if directory in seen_directories:
                    
                    
                    continue
                seen_directories.add(directory)

                files = []
                subdirs = []
                for name in sorted(os.listdir(directory)):
                    path = os.path.join(directory, name)
                    if os.path.isfile(path):
                        
                        
                        if accept_filename(name):
                            files.append(name)
                        continue
                    elif os.path.islink(path):
                        
                        path = os.path.realpath(path)

                    
                    if accept_dirname(name):
                        subdirs.append(name)
                        
                        directories.insert(0, path)

                
                
                if subdirs or files:
                    callback(rootdirectory, directory, subdirs, files)


    @classmethod
    def populate_directory_manifests(cls, directories, filename, pattern=None, ignore=(), overwrite=False):
        """
        walks directories and writes manifests of name `filename` in-place; returns `cls` instance populated
        with the given manifests

        filename -- filename of manifests to write
        pattern -- shell pattern (glob) or patterns of filenames to match
        ignore -- directory names to ignore
        overwrite -- whether to overwrite existing files of given name
        """

        manifest_dict = {}

        if os.path.basename(filename) != filename:
            raise IOError("filename should not include directory name")

        
        _directories = directories
        directories = []
        for directory in _directories:
            if directory not in directories:
                directories.append(directory)

        def callback(directory, dirpath, dirnames, filenames):
            """write a manifest for each directory"""

            manifest_path = os.path.join(dirpath, filename)
            if (dirnames or filenames) and not (os.path.exists(manifest_path) and overwrite):
                with file(manifest_path, 'w') as manifest:
                    for dirname in dirnames:
                        print >> manifest, '[include:%s]' % os.path.join(dirname, filename)
                    for _filename in filenames:
                        print >> manifest, '[%s]' % _filename

                
                manifest_dict.setdefault(directory, manifest_path)

        
        cls._walk_directories(directories, callback, pattern=pattern, ignore=ignore)
        
        manifests = [manifest_dict[directory] for directory in _directories]

        
        return cls(manifests=manifests)

    @classmethod
    def from_directories(cls, directories, pattern=None, ignore=(), write=None, relative_to=None):
        """
        convert directories to a simple manifest; returns ManifestParser instance

        pattern -- shell pattern (glob) or patterns of filenames to match
        ignore -- directory names to ignore
        write -- filename or file-like object of manifests to write;
                 if `None` then a StringIO instance will be created
        relative_to -- write paths relative to this path;
                       if false then the paths are absolute
        """


        
        opened_manifest_file = None 
        absolute = not relative_to 
        if isinstance(write, string):
            opened_manifest_file = write
            write = file(write, 'w')
        if write is None:
            write = StringIO()

        
        def callback(directory, dirpath, dirnames, filenames):

            
            filenames = [os.path.join(dirpath, filename)
                         for filename in filenames]
            
            filenames = [filename for filename in filenames
                         if filename != opened_manifest_file]
            
            if not absolute and relative_to:
                filenames = [relpath(filename, relative_to)
                             for filename in filenames]

            
            print >> write, '\n'.join(['[%s]' % denormalize_path(filename)
                                               for filename in filenames])


        cls._walk_directories(directories, callback, pattern=pattern, ignore=ignore)

        if opened_manifest_file:
            
            write.close()
            manifests = [opened_manifest_file]
        else:
            
            
            write.flush()
            write.seek(0)
            manifests = [write]


        
        return cls(manifests=manifests)

convert = ManifestParser.from_directories


class TestManifest(ManifestParser):
    """
    apply logic to manifests;  this is your integration layer :)
    specific harnesses may subclass from this if they need more logic
    """

    def __init__(self, *args, **kwargs):
        ManifestParser.__init__(self, *args, **kwargs)
        self.filters = filterlist(DEFAULT_FILTERS)

    def active_tests(self, exists=True, disabled=True, filters=None, **values):
        """
        Run all applied filters on the set of tests.

        :param exists: filter out non-existing tests (default True)
        :param disabled: whether to return disabled tests (default True)
        :param values: keys and values to filter on (e.g. `os = linux mac`)
        :param filters: list of filters to apply to the tests
        :returns: list of test objects that were not filtered out
        """
        tests = [i.copy() for i in self.tests] 

        
        for test in tests:
            test['expected'] = test.get('expected', 'pass')

        
        fltrs = self.filters[:]
        if exists:
            if self.strict:
                self.check_missing(tests)
            else:
                fltrs.append(_exists)

        if not disabled:
            fltrs.append(enabled)

        if filters:
            fltrs += filters

        for fn in fltrs:
            tests = fn(tests, values)
        return list(tests)

    def test_paths(self):
        return [test['path'] for test in self.active_tests()]
