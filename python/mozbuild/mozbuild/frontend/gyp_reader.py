



from __future__ import unicode_literals
import gyp
import sys
import time
import os
import mozpack.path as mozpath
from mozpack.files import FileFinder
from .sandbox import (
    alphabetical_sorted,
    GlobalNamespace,
)
from .sandbox_symbols import VARIABLES
from .reader import SandboxValidationError



sys.modules['gyp.generator.mozbuild'] = sys.modules[__name__]








chrome_src = mozpath.abspath(mozpath.join(mozpath.dirname(gyp.__file__),
    '../../../..'))
script_dir = mozpath.join(chrome_src, 'build')


generator_default_variables = {
}
for dirname in ['INTERMEDIATE_DIR', 'SHARED_INTERMEDIATE_DIR', 'PRODUCT_DIR',
                'LIB_DIR', 'SHARED_LIB_DIR']:
  
  generator_default_variables[dirname] = b'dir'

for unused in ['RULE_INPUT_PATH', 'RULE_INPUT_ROOT', 'RULE_INPUT_NAME',
               'RULE_INPUT_DIRNAME', 'RULE_INPUT_EXT',
               'EXECUTABLE_PREFIX', 'EXECUTABLE_SUFFIX',
               'STATIC_LIB_PREFIX', 'STATIC_LIB_SUFFIX',
               'SHARED_LIB_PREFIX', 'SHARED_LIB_SUFFIX',
               'LINKER_SUPPORTS_ICF']:
  generator_default_variables[unused] = b''


class GypSandbox(GlobalNamespace):
    """Class mimicking MozbuildSandbox for processing of the data
    extracted from Gyp by a mozbuild backend.

    Inherits from GlobalNamespace because it doesn't need the extra
    functionality from Sandbox.
    """
    def __init__(self, main_path, dependencies_paths=[]):
        self.main_path = main_path
        self.all_paths = set([main_path]) | set(dependencies_paths)
        self.execution_time = 0
        GlobalNamespace.__init__(self, allowed_variables=VARIABLES)

    def get_affected_tiers(self):
        tiers = (VARIABLES[key][3] for key in self if key in VARIABLES)
        return set(tier for tier in tiers if tier)


def encode(value):
    if isinstance(value, unicode):
        return value.encode('utf-8')
    return value


def read_from_gyp(config, path, output, vars, non_unified_sources = set()):
    """Read a gyp configuration and emits GypSandboxes for the backend to
    process.

    config is a ConfigEnvironment, path is the path to a root gyp configuration
    file, output is the base path under which the objdir for the various gyp
    dependencies will be, and vars a dict of variables to pass to the gyp
    processor.
    """

    time_start = time.time()
    all_sources = set()

    
    
    path = encode(path)
    str_vars = dict((name, encode(value)) for name, value in vars.items())

    params = {
        b'parallel': False,
        b'generator_flags': {},
        b'build_files': [path],
    }

    
    includes = [encode(mozpath.join(script_dir, 'common.gypi'))]
    finder = FileFinder(chrome_src, find_executables=False)
    includes.extend(encode(mozpath.join(chrome_src, name))
        for name, _ in finder.find('*/supplement.gypi'))

    
    generator, flat_list, targets, data = \
        gyp.Load([path], format=b'mozbuild',
            default_variables=str_vars,
            includes=includes,
            depth=encode(mozpath.dirname(path)),
            params=params)

    
    
    
    for target in gyp.common.AllTargets(flat_list, targets, path.replace(b'/', os.sep)):
        build_file, target_name, toolset = gyp.common.ParseQualifiedTarget(target)
        
        included_files = [mozpath.abspath(mozpath.join(mozpath.dirname(build_file), f))
                          for f in data[build_file]['included_files']]
        
        sandbox = GypSandbox(mozpath.abspath(build_file), included_files)
        sandbox.config = config

        with sandbox.allow_all_writes() as d:
            topsrcdir = config.topsrcdir
            relsrcdir = d['RELATIVEDIR'] = mozpath.relpath(mozpath.dirname(build_file), config.topsrcdir)
            d['SRCDIR'] = mozpath.join(topsrcdir, relsrcdir)

            
            
            
            
            
            
            reldir  = mozpath.relpath(mozpath.dirname(build_file),
                                      mozpath.dirname(path))
            subdir = '%s_%s' % (
                mozpath.splitext(mozpath.basename(build_file))[0],
                target_name,
            )
            d['OBJDIR'] = mozpath.join(output, reldir, subdir)
            d['IS_GYP_DIR'] = True

        spec = targets[target]

        
        c = 'Debug' if config.substs['MOZ_DEBUG'] else 'Release'
        if c not in spec['configurations']:
            raise RuntimeError('Missing %s gyp configuration for target %s '
                               'in %s' % (c, target_name, build_file))
        target_conf = spec['configurations'][c]

        if spec['type'] == 'none':
            continue
        elif spec['type'] == 'static_library':
            sandbox['FORCE_STATIC_LIB'] = True
            
            
            name = spec['target_name']
            if name.startswith('lib'):
                name = name[3:]
            
            sandbox['LIBRARY_NAME'] = name.decode('utf-8')
            
            sources = set(mozpath.normpath(mozpath.join(sandbox['SRCDIR'], f))
                for f in spec.get('sources', [])
                if mozpath.splitext(f)[-1] != '.h')
            asm_sources = set(f for f in sources if f.endswith('.S'))

            unified_sources = sources - non_unified_sources - asm_sources
            sources -= unified_sources
            all_sources |= sources
            
            sandbox['SOURCES'] = alphabetical_sorted(sources)
            sandbox['UNIFIED_SOURCES'] = alphabetical_sorted(unified_sources)

            for define in target_conf.get('defines', []):
                if '=' in define:
                    name, value = define.split('=', 1)
                    sandbox['DEFINES'][name] = value
                else:
                    sandbox['DEFINES'][define] = True

            for include in target_conf.get('include_dirs', []):
                sandbox['LOCAL_INCLUDES'] += [include]

            with sandbox.allow_all_writes() as d:
                d['EXTRA_ASSEMBLER_FLAGS'] = target_conf.get('asflags_mozilla', [])
                d['EXTRA_COMPILE_FLAGS'] = target_conf.get('cflags_mozilla', [])
        else:
            
            
            
            raise NotImplementedError('Unsupported gyp target type: %s' % spec['type'])

        sandbox.execution_time = time.time() - time_start
        yield sandbox
        time_start = time.time()




