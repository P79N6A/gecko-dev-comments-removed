






































"""
Mozilla universal manifest parser
"""




__all__ = ['read_ini', 
           'ManifestParser', 'TestManifest', 'convert', 
           'parse', 'ParseError', 'ExpressionParser'] 

import os
import re
import shutil
import sys
from fnmatch import fnmatch
from optparse import OptionParser

version = '0.5.1' 
try:
    from setuptools import setup
except ImportError:
    setup = None



try:
    relpath = os.path.relpath
except AttributeError:
    def relpath(path, start):
        """
        Return a relative version of a path
        from /usr/lib/python2.6/posixpath.py
        """

        if not path:
            raise ValueError("no path specified")

        start_list = os.path.abspath(start).split(os.path.sep)
        path_list = os.path.abspath(path).split(os.path.sep)

        
        i = len(os.path.commonprefix([start_list, path_list]))

        rel_list = [os.path.pardir] * (len(start_list)-i) + path_list[i:]
        if not rel_list:
            return start
        return os.path.join(*rel_list)


































class ident_token(object):
    def __init__(self, value):
        self.value = value
    def nud(self, parser):
        
        
        return parser.value(self.value)

class literal_token(object):
    def __init__(self, value):
        self.value = value
    def nud(self, parser):
        return self.value

class eq_op_token(object):
    "=="
    def led(self, parser, left):
        return left == parser.expression(self.lbp)
    
class neq_op_token(object):
    "!="
    def led(self, parser, left):
        return left != parser.expression(self.lbp)

class not_op_token(object):
    "!"
    def nud(self, parser):
        return not parser.expression()

class and_op_token(object):
    "&&"
    def led(self, parser, left):
        right = parser.expression(self.lbp)
        return left and right
    
class or_op_token(object):
    "||"
    def led(self, parser, left):
        right = parser.expression(self.lbp)
        return left or right

class lparen_token(object):
    "("
    def nud(self, parser):
        expr = parser.expression()
        parser.advance(rparen_token)
        return expr

class rparen_token(object):
    ")"

class end_token(object):
    """always ends parsing"""



class bool_token(literal_token):
    def __init__(self, value):
        value = {'true':True, 'false':False}[value]
        literal_token.__init__(self, value)

class int_token(literal_token):
    def __init__(self, value):
        literal_token.__init__(self, int(value))

class string_token(literal_token):
    def __init__(self, value):
        literal_token.__init__(self, value[1:-1])

precedence = [(end_token, rparen_token),
              (or_op_token,),
              (and_op_token,),
              (eq_op_token, neq_op_token),
              (lparen_token,),
              ]
for index, rank in enumerate(precedence):
    for token in rank:
        token.lbp = index 

class ParseError(Exception):
    """errror parsing conditional expression"""

class ExpressionParser(object):
    def __init__(self, text, valuemapping, strict=False):
        """
        Initialize the parser with input |text|, and |valuemapping| as
        a dict mapping identifier names to values.
        """
        self.text = text
        self.valuemapping = valuemapping
        self.strict = strict

    def _tokenize(self):
        """
        Lex the input text into tokens and yield them in sequence.
        """
        
        def bool_(scanner, t): return bool_token(t)
        def identifier(scanner, t): return ident_token(t)
        def integer(scanner, t): return int_token(t)
        def eq(scanner, t): return eq_op_token()
        def neq(scanner, t): return neq_op_token()
        def or_(scanner, t): return or_op_token()
        def and_(scanner, t): return and_op_token()
        def lparen(scanner, t): return lparen_token()
        def rparen(scanner, t): return rparen_token()
        def string_(scanner, t): return string_token(t)
        def not_(scanner, t): return not_op_token()

        scanner = re.Scanner([
            (r"true|false", bool_),
            (r"[a-zA-Z_]\w*", identifier),
            (r"[0-9]+", integer),
            (r'("[^"]*")|(\'[^\']*\')', string_),
            (r"==", eq),
            (r"!=", neq),
            (r"\|\|", or_),
            (r"!", not_),
            (r"&&", and_),
            (r"\(", lparen),
            (r"\)", rparen),
            (r"\s+", None), 
            ])
        tokens, remainder = scanner.scan(self.text)
        for t in tokens:
            yield t
        yield end_token()

    def value(self, ident):
        """
        Look up the value of |ident| in the value mapping passed in the
        constructor.
        """
        if self.strict:
            return self.valuemapping[ident]
        else:
            return self.valuemapping.get(ident, None)

    def advance(self, expected):
        """
        Assert that the next token is an instance of |expected|, and advance
        to the next token.
        """
        if not isinstance(self.token, expected):
            raise Exception, "Unexpected token!"
        self.token = self.iter.next()
        
    def expression(self, rbp=0):
        """
        Parse and return the value of an expression until a token with
        right binding power greater than rbp is encountered.
        """
        t = self.token
        self.token = self.iter.next()
        left = t.nud(self)
        while rbp < self.token.lbp:
            t = self.token
            self.token = self.iter.next()
            left = t.led(self, left)
        return left

    def parse(self):
        """
        Parse and return the value of the expression in the text
        passed to the constructor. Raises a ParseError if the expression
        could not be parsed.
        """
        try:
            self.iter = self._tokenize()
            self.token = self.iter.next()
            return self.expression()
        except:
            raise ParseError("could not parse: %s; variables: %s" % (self.text, self.valuemapping))

    __call__ = parse

def parse(text, **values):
    """
    Parse and evaluate a boolean expression in |text|. Use |values| to look
    up the value of identifiers referenced in the expression. Returns the final
    value of the expression. A ParseError will be raised if parsing fails.
    """
    return ExpressionParser(text, values).parse()

def normalize_path(path):
    """normalize a relative path"""
    if sys.platform.startswith('win'):
        return path.replace('/', os.path.sep)
    return path

def read_ini(fp, variables=None, default='DEFAULT',
             comments=';#', separators=('=', ':'),
             strict=True):
    """
    read an .ini file and return a list of [(section, values)]
    - fp : file pointer or path to read
    - variables : default set of variables
    - default : name of the section for the default section
    - comments : characters that if they start a line denote a comment
    - separators : strings that denote key, value separation in order
    - strict : whether to be strict about parsing
    """

    if variables is None:
        variables = {}

    if isinstance(fp, basestring):
        fp = file(fp)

    sections = []
    key = value = None
    section_names = set([])

    
    for line in fp.readlines():

        stripped = line.strip()

        
        if not stripped:
            
            key = value = None
            continue

        
        if stripped[0] in comments:
            continue

        
        if len(stripped) > 2 and stripped[0] == '[' and stripped[-1] == ']':
            section = stripped[1:-1].strip()
            key = value = None

            
            if section.lower() == default.lower():
                if strict:
                    assert default not in section_names
                section_names.add(default)
                current_section = variables
                continue

            if strict:
                
                assert section not in section_names

            section_names.add(section)
            current_section = {}
            sections.append((section, current_section))
            continue

        
        if not section_names:
            raise Exception('No sections found')

        
        for separator in separators:
            if separator in stripped:
                key, value = stripped.split(separator, 1)
                key = key.strip()
                value = value.strip()

                if strict:
                    
                    assert key
                    if current_section is not variables:
                        assert key not in current_section

                current_section[key] = value
                break
        else:
            
            if line[0].isspace() and key:
                value = '%s%s%s' % (value, os.linesep, stripped)
                current_section[key] = value
            else:
                
                raise Exception("Not sure what you're trying to do")

    
    def interpret_variables(global_dict, local_dict):
        variables = global_dict.copy()
        variables.update(local_dict)
        return variables

    sections = [(i, interpret_variables(variables, j)) for i, j in sections]
    return sections




class ManifestParser(object):
    """read .ini manifests"""

    

    def __init__(self, manifests=(), defaults=None, strict=True):
        self._defaults = defaults or {}
        self.tests = []
        self.strict = strict
        self.rootdir = None
        self.relativeRoot = None
        if manifests:
            self.read(*manifests)

    def getRelativeRoot(self, root):
        return root

    def read(self, *filenames, **defaults):

        
        missing = [ filename for filename in filenames
                    if not os.path.exists(filename) ]
        if missing:
            raise IOError('Missing files: %s' % ', '.join(missing))

        
        for filename in filenames:

            
            defaults = defaults.copy() or self._defaults.copy()
            here = os.path.dirname(os.path.abspath(filename))
            defaults['here'] = here

            if self.rootdir is None:
                
                
                self.rootdir = here

            
            sections = read_ini(fp=filename, variables=defaults, strict=self.strict)

            
            for section, data in sections:

                
                
                
                if section.startswith('include:'):
                    include_file = section.split('include:', 1)[-1]
                    include_file = normalize_path(include_file)
                    if not os.path.isabs(include_file):
                        include_file = os.path.join(self.getRelativeRoot(here), include_file)
                    if not os.path.exists(include_file):
                        if self.strict:
                            raise IOError("File '%s' does not exist" % include_file)
                        else:
                            continue
                    include_defaults = data.copy()
                    self.read(include_file, **include_defaults)
                    continue

                
                test = data
                test['name'] = section
                test['manifest'] = os.path.abspath(filename)

                
                path = test.get('path', section)
                if '://' not in path: 
                    path = normalize_path(path)
                    if not os.path.isabs(path):
                        path = os.path.join(here, path)
                test['path'] = path

                
                self.tests.append(test)

    

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

    def missing(self, tests=None):
        """return list of tests that do not exist on the filesystem"""
        if tests is None:
            tests = self.tests
        return [test for test in tests
                if not os.path.exists(test['path'])]

    def manifests(self, tests=None):
        """
        return manifests in order in which they appear in the tests
        """
        if tests is None:
            tests = self.tests
        manifests = []
        for test in tests:
            manifest = test.get('manifest')
            if not manifest:
                continue
            if manifest not in manifests:
                manifests.append(manifest)
        return manifests

    

    def write(self, fp=sys.stdout, rootdir=None,
              global_tags=None, global_kwargs=None,
              local_tags=None, local_kwargs=None):
        """
        write a manifest given a query
        global and local options will be munged to do the query
        globals will be written to the top of the file
        locals (if given) will be written per test
        """

        
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
                path = relpath(test['path'], self.rootdir)
            print >> fp, '[%s]' % path
          
            
            reserved = ['path', 'name', 'here', 'manifest']
            for key in sorted(test.keys()):
                if key in reserved:
                    continue
                if key in global_kwargs:
                    continue
                if key in global_tags and not test[key]:
                    continue
                print >> fp, '%s = %s' % (key, test[key])
            print >> fp

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
        for test in tests:
            if os.path.isabs(test['name']):
                continue
            source = test['path']
            if not os.path.exists(source):
                print >> sys.stderr, "Missing test: '%s' does not exist!" % source
                continue
                
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
                    
                    print >> sys.stderr, "Missing test: '%s'; skipping" % test['name']
                    continue
                destination = os.path.join(rootdir, _relpath)
                shutil.copy(source, destination)


class TestManifest(ManifestParser):
    """
    apply logic to manifests;  this is your integration layer :)
    specific harnesses may subclass from this if they need more logic
    """

    def filter(self, values, tests):
        """
        filter on a specific list tag, e.g.:
        run-if.os = win linux
        skip-if.os = mac
        """

        
        run_tag = 'run-if'
        skip_tag = 'skip-if'
        fail_tag = 'fail-if'

        
        for test in tests:
            reason = None 
            
            
            if run_tag in test:
                condition = test[run_tag]
                if not parse(condition, **values):
                    reason = '%s: %s' % (run_tag, condition)

            
            if skip_tag in test:
                condition = test[skip_tag]
                if parse(condition, **values):
                    reason = '%s: %s' % (skip_tag, condition)

            
            if reason:
                test.setdefault('disabled', reason)        

            
            if fail_tag in test:
                condition = test[fail_tag]
                if parse(condition, **values):
                    test['expected'] = 'fail'

    def active_tests(self, exists=True, disabled=True, **values):
        """
        - exists : return only existing tests
        - disabled : whether to return disabled tests
        - tags : keys and values to filter on (e.g. `os = linux mac`)
        """

        tests = [i.copy() for i in self.tests] 

        
        for test in tests:
            test['expected'] = test.get('expected', 'pass')
        
        
        if exists:
            tests = [test for test in tests if os.path.exists(test['path'])]

        
        self.filter(values, tests)

        
        if not disabled:
            tests = [test for test in tests
                     if not 'disabled' in test]

        
        return tests

    def test_paths(self):
        return [test['path'] for test in self.active_tests()]




def convert(directories, pattern=None, ignore=(), write=None):
    """
    convert directories to a simple manifest
    """

    retval = []
    include = []
    for directory in directories:
        for dirpath, dirnames, filenames in os.walk(directory):

            
            dirnames = [ i for i in dirnames if i not in ignore ]
            dirnames.sort()

            
            _dirpath = dirpath
            dirpath = dirpath.split(directory, 1)[-1].strip('/')

            if dirpath.split(os.path.sep)[0] in ignore:
                continue

            
            if pattern:
                filenames = [filename for filename in filenames
                             if fnmatch(filename, pattern)]

            filenames.sort()

            
            if write and (dirnames or filenames):
                manifest = file(os.path.join(_dirpath, write), 'w')
                for dirname in dirnames:
                    print >> manifest, '[include:%s]' % os.path.join(dirname, write)
                for filename in filenames:
                    print >> manifest, '[%s]' % filename
                manifest.close()

            
            retval.extend([os.path.join(dirpath, filename)
                           for filename in filenames])

    if write:
        return 
  
    retval.sort()
    retval = ['[%s]' % filename for filename in retval]
    return '\n'.join(retval)



class ParserError(Exception):
  """error for exceptions while parsing the command line"""

def parse_args(_args):
    """
    parse and return:
    --keys=value (or --key value)
    -tags
    args
    """

    
    _dict = {}
    tags = []
    args = []

    
    key = None
    for arg in _args:
        if arg.startswith('---'):
            raise ParserError("arguments should start with '-' or '--' only")
        elif arg.startswith('--'):
            if key:
                raise ParserError("Key %s still open" % key)
            key = arg[2:]
            if '=' in key:
                key, value = key.split('=', 1)
                _dict[key] = value
                key = None
                continue
        elif arg.startswith('-'):
            if key:
                raise ParserError("Key %s still open" % key)
            tags.append(arg[1:])
            continue
        else:
            if key:
                _dict[key] = arg
                continue
            args.append(arg)

    
    return (_dict, tags, args)




class CLICommand(object):
    usage = '%prog [options] command'
    def __init__(self, parser):
      self._parser = parser 
    def parser(self):
      return OptionParser(usage=self.usage, description=self.__doc__,
                          add_help_option=False)

class Copy(CLICommand):
    usage = '%prog [options] copy manifest directory -tag1 -tag2 --key1=value1 --key2=value2 ...'
    def __call__(self, options, args):
      
      try:
        kwargs, tags, args = parse_args(args)
      except ParserError, e:
        self._parser.error(e.message)

      
      
      if not len(args) == 2:
        HelpCLI(self._parser)(options, ['copy'])
        return

      
      
      manifests = ManifestParser()
      manifests.read(args[0])

      
      manifests.copy(args[1], None, *tags, **kwargs)


class CreateCLI(CLICommand):
    """
    create a manifest from a list of directories
    """
    usage = '%prog [options] create directory <directory> <...>'

    def parser(self):
        parser = CLICommand.parser(self)
        parser.add_option('-p', '--pattern', dest='pattern',
                          help="glob pattern for files")
        parser.add_option('-i', '--ignore', dest='ignore',
                          default=[], action='append',
                          help='directories to ignore')
        parser.add_option('-w', '--in-place', dest='in_place',
                          help='Write .ini files in place; filename to write to')
        return parser

    def __call__(self, _options, args):
        parser = self.parser()
        options, args = parser.parse_args(args)

        
        if not len(args):
            parser.print_usage()
            return

        
        for arg in args:
            assert os.path.exists(arg)
            assert os.path.isdir(arg)
            manifest = convert(args, pattern=options.pattern, ignore=options.ignore,
                               write=options.in_place)
        if manifest:
            print manifest


class WriteCLI(CLICommand):
    """
    write a manifest based on a query
    """
    usage = '%prog [options] write manifest <manifest> -tag1 -tag2 --key1=value1 --key2=value2 ...'
    def __call__(self, options, args):

        
        try:
            kwargs, tags, args = parse_args(args)
        except ParserError, e:
            self._parser.error(e.message)

        
        
        if not args:
            HelpCLI(self._parser)(options, ['write'])
            return

        
        
        manifests = ManifestParser()
        manifests.read(*args)

        
        manifests.write(global_tags=tags, global_kwargs=kwargs)
      

class HelpCLI(CLICommand):
    """
    get help on a command
    """
    usage = '%prog [options] help [command]'

    def __call__(self, options, args):
        if len(args) == 1 and args[0] in commands:
            commands[args[0]](self._parser).parser().print_help()
        else:
            self._parser.print_help()
            print '\nCommands:'
            for command in sorted(commands):
                print '  %s : %s' % (command, commands[command].__doc__.strip())

class SetupCLI(CLICommand):
    """
    setup using setuptools
    """
    
    
    
    
    usage = '%prog [options] setup [setuptools options]'
    
    def __call__(self, options, args):
        sys.argv = [sys.argv[0]] + args
        assert setup is not None, "You must have setuptools installed to use SetupCLI"
        here = os.path.dirname(os.path.abspath(__file__))
        try:
            filename = os.path.join(here, 'README.txt')
            description = file(filename).read()
        except:    
            description = ''
        os.chdir(here)

        setup(name='ManifestDestiny',
              version=version,
              description="Universal manifests for Mozilla test harnesses",
              long_description=description,
              classifiers=[], 
              keywords='mozilla manifests',
              author='Jeff Hammel',
              author_email='jhammel@mozilla.com',
              url='https://wiki.mozilla.org/Auto-tools/Projects/ManifestDestiny',
              license='MPL',
              zip_safe=False,
              py_modules=['manifestparser'],
              install_requires=[
                  
                  ],
              entry_points="""
              [console_scripts]
              manifestparser = manifestparser:main
              """,
              )


class UpdateCLI(CLICommand):
    """
    update the tests as listed in a manifest from a directory
    """
    usage = '%prog [options] update manifest directory -tag1 -tag2 --key1=value1 --key2=value2 ...'

    def __call__(self, options, args):
        
        try:
            kwargs, tags, args = parse_args(args)
        except ParserError, e:
            self._parser.error(e.message)

        
        
        if not len(args) == 2:
            HelpCLI(self._parser)(options, ['update'])
            return

        
        
        manifests = ManifestParser()
        manifests.read(args[0])

        
        manifests.update(args[1], None, *tags, **kwargs)



commands = { 'create': CreateCLI,
             'help': HelpCLI,
             'update': UpdateCLI,
             'write': WriteCLI }
if setup is not None:
    commands['setup'] = SetupCLI

def main(args=sys.argv[1:]):
    """console_script entry point"""

    
    usage = '%prog [options] [command] ...'
    description = __doc__
    parser = OptionParser(usage=usage, description=description)
    parser.add_option('-s', '--strict', dest='strict',
                      action='store_true', default=False,
                      help='adhere strictly to errors')
    parser.disable_interspersed_args()

    options, args = parser.parse_args(args)

    if not args:
        HelpCLI(parser)(options, args)
        parser.exit()

    
    command = args[0]
    if command not in commands:
        parser.error("Command must be one of %s (you gave '%s')" % (', '.join(sorted(commands.keys())), command))

    handler = commands[command](parser)
    handler(options, args[1:])

if __name__ == '__main__':
    main()
