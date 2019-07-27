



from __future__ import unicode_literals
import gyp
import sys
import time
import os
import mozpack.path as mozpath
from mozpack.files import FileFinder
from .sandbox import alphabetical_sorted
from .context import (
    TemplateContext,
    VARIABLES,
)
from mozbuild.util import (
    List,
    memoize,
)
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


class GypContext(TemplateContext):
    """Specialized Context for use with data extracted from Gyp.

    config is the ConfigEnvironment for this context.
    relobjdir is the object directory that will be used for this context,
    relative to the topobjdir defined in the ConfigEnvironment.
    """
    def __init__(self, config, relobjdir):
        self._relobjdir = relobjdir
        TemplateContext.__init__(self, allowed_variables=self.VARIABLES(),
            config=config)

    @classmethod
    @memoize
    def VARIABLES(cls):
        """Returns the allowed variables for a GypContext."""
        
        
        return dict(VARIABLES,
        IS_GYP_DIR=(bool, bool, '', None),
        EXTRA_ASSEMBLER_FLAGS=(List, list, '', None),
        EXTRA_COMPILE_FLAGS=(List, list, '', None),
    )


def encode(value):
    if isinstance(value, unicode):
        return value.encode('utf-8')
    return value


def read_from_gyp(config, path, output, vars, non_unified_sources = set()):
    """Read a gyp configuration and emits GypContexts for the backend to
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

        
        
        
        
        
        
        reldir  = mozpath.relpath(mozpath.dirname(build_file),
                                  mozpath.dirname(path))
        subdir = '%s_%s' % (
            mozpath.splitext(mozpath.basename(build_file))[0],
            target_name,
        )
        
        context = GypContext(config, mozpath.relpath(
            mozpath.join(output, reldir, subdir), config.topobjdir))
        context.add_source(mozpath.abspath(build_file))
        
        for f in data[build_file]['included_files']:
            context.add_source(mozpath.abspath(mozpath.join(
                mozpath.dirname(build_file), f)))

        context['IS_GYP_DIR'] = True

        spec = targets[target]

        
        c = 'Debug' if config.substs['MOZ_DEBUG'] else 'Release'
        if c not in spec['configurations']:
            raise RuntimeError('Missing %s gyp configuration for target %s '
                               'in %s' % (c, target_name, build_file))
        target_conf = spec['configurations'][c]

        if spec['type'] == 'none':
            continue
        elif spec['type'] == 'static_library':
            
            
            name = spec['target_name']
            if name.startswith('lib'):
                name = name[3:]
            
            context['LIBRARY_NAME'] = name.decode('utf-8')
            
            sources = set(mozpath.normpath(mozpath.join(context.srcdir, f))
                for f in spec.get('sources', [])
                if mozpath.splitext(f)[-1] != '.h')
            asm_sources = set(f for f in sources if f.endswith('.S'))

            unified_sources = sources - non_unified_sources - asm_sources
            sources -= unified_sources
            all_sources |= sources
            
            context['SOURCES'] = alphabetical_sorted(sources)
            context['UNIFIED_SOURCES'] = alphabetical_sorted(unified_sources)

            for define in target_conf.get('defines', []):
                if '=' in define:
                    name, value = define.split('=', 1)
                    context['DEFINES'][name] = value
                else:
                    context['DEFINES'][define] = True

            for include in target_conf.get('include_dirs', []):
                
                
                
                
                
                
                
                
                if include.startswith('/'):
                    resolved = mozpath.abspath(mozpath.join(config.topsrcdir, include[1:]))
                else:
                    resolved = mozpath.abspath(mozpath.join(mozpath.dirname(build_file), include))
                if not os.path.exists(resolved):
                    continue
                context['LOCAL_INCLUDES'] += [include]

            context['EXTRA_ASSEMBLER_FLAGS'] = target_conf.get('asflags_mozilla', [])
            context['EXTRA_COMPILE_FLAGS'] = target_conf.get('cflags_mozilla', [])
        else:
            
            
            
            raise NotImplementedError('Unsupported gyp target type: %s' % spec['type'])

        
        
        context['LOCAL_INCLUDES'] += [
            '/ipc/chromium/src',
            '/ipc/glue',
        ]
        context['GENERATED_INCLUDES'] += ['/ipc/ipdl/_ipdlheaders']
        
        if config.substs['OS_TARGET'] == 'WINNT':
            context['DEFINES']['UNICODE'] = True
            context['DEFINES']['_UNICODE'] = True
        context['DISABLE_STL_WRAPPING'] = True

        context.execution_time = time.time() - time_start
        yield context
        time_start = time.time()




