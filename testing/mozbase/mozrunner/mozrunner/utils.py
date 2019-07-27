





"""Utility functions for mozrunner"""

__all__ = ['findInPath', 'get_metadata_from_egg', 'uses_marionette']


from functools import wraps
import mozinfo
import os
import sys



try:
    import pkg_resources
    def get_metadata_from_egg(module):
        ret = {}
        try:
            dist = pkg_resources.get_distribution(module)
        except pkg_resources.DistributionNotFound:
            return {}
        if dist.has_metadata("PKG-INFO"):
            key = None
            for line in dist.get_metadata("PKG-INFO").splitlines():
                
                if key == 'Description':
                    
                    if not line or line[0].isspace():
                        value += '\n' + line
                        continue
                    else:
                        key = key.strip()
                        value = value.strip()
                        ret[key] = value

                key, value = line.split(':', 1)
                key = key.strip()
                value = value.strip()
                ret[key] = value
        if dist.has_metadata("requires.txt"):
            ret["Dependencies"] = "\n" + dist.get_metadata("requires.txt")
        return ret
except ImportError:
    
    def get_metadata_from_egg(module):
        return {}


def findInPath(fileName, path=os.environ['PATH']):
    """python equivalent of which; should really be in the stdlib"""
    dirs = path.split(os.pathsep)
    for dir in dirs:
        if os.path.isfile(os.path.join(dir, fileName)):
            return os.path.join(dir, fileName)
        if mozinfo.isWin:
            if os.path.isfile(os.path.join(dir, fileName + ".exe")):
                return os.path.join(dir, fileName + ".exe")

if __name__ == '__main__':
    for i in sys.argv[1:]:
        print findInPath(i)


def _find_marionette_in_args(*args, **kwargs):
    try:
        m = [a for a in args + tuple(kwargs.values()) if hasattr(a, 'session')][0]
    except IndexError:
        print("Can only apply decorator to function using a marionette object")
        raise
    return m

def uses_marionette(func):
    """Decorator which creates a marionette session and deletes it
    afterwards if one doesn't already exist.
    """
    @wraps(func)
    def _(*args, **kwargs):
        m = _find_marionette_in_args(*args, **kwargs)
        delete_session = False
        if not m.session:
            delete_session = True
            m.start_session()

        m.set_context(m.CONTEXT_CHROME)
        ret = func(*args, **kwargs)

        if delete_session:
            m.delete_session()

        return ret
    return _


def _raw_log():
    import logging
    return logging.getLogger(__name__)


def test_environment(xrePath, env=None, crashreporter=True, debugger=False,
                     dmdPath=None, lsanPath=None, log=None):
    """
    populate OS environment variables for mochitest and reftests.

    Originally comes from automationutils.py. Don't use that for new code.
    """

    env = os.environ.copy() if env is None else env
    log = log or _raw_log()

    assert os.path.isabs(xrePath)

    if mozinfo.isMac:
        ldLibraryPath = os.path.join(os.path.dirname(xrePath), "MacOS")
    else:
        ldLibraryPath = xrePath

    envVar = None
    dmdLibrary = None
    preloadEnvVar = None
    if 'toolkit' in mozinfo.info and mozinfo.info['toolkit'] == "gonk":
        
        pass
    elif mozinfo.isUnix:
        envVar = "LD_LIBRARY_PATH"
        env['MOZILLA_FIVE_HOME'] = xrePath
        dmdLibrary = "libdmd.so"
        preloadEnvVar = "LD_PRELOAD"
    elif mozinfo.isMac:
        envVar = "DYLD_LIBRARY_PATH"
        dmdLibrary = "libdmd.dylib"
        preloadEnvVar = "DYLD_INSERT_LIBRARIES"
    elif mozinfo.isWin:
        envVar = "PATH"
        dmdLibrary = "dmd.dll"
        preloadEnvVar = "MOZ_REPLACE_MALLOC_LIB"
    if envVar:
        envValue = ((env.get(envVar), str(ldLibraryPath))
                    if mozinfo.isWin
                    else (ldLibraryPath, dmdPath, env.get(envVar)))
        env[envVar] = os.path.pathsep.join([path for path in envValue if path])

    if dmdPath and dmdLibrary and preloadEnvVar:
        env[preloadEnvVar] = os.path.join(dmdPath, dmdLibrary)

    
    env['GNOME_DISABLE_CRASH_DIALOG'] = '1'
    env['XRE_NO_WINDOWS_CRASH_DIALOG'] = '1'

    if crashreporter and not debugger:
        env['MOZ_CRASHREPORTER_NO_REPORT'] = '1'
        env['MOZ_CRASHREPORTER'] = '1'
    else:
        env['MOZ_CRASHREPORTER_DISABLE'] = '1'

    
    
    
    
    env.setdefault('MOZ_DISABLE_NONLOCAL_CONNECTIONS', '1')

    
    env.setdefault(
        'NSPR_LOG_MODULES',
        'signaling:3,mtransport:4,datachannel:4,jsep:4,MediaPipelineFactory:4'
    )
    env.setdefault('R_LOG_LEVEL', '6')
    env.setdefault('R_LOG_DESTINATION', 'stderr')
    env.setdefault('R_LOG_VERBOSE', '1')

    
    asan = bool(mozinfo.info.get("asan"))
    if asan and (mozinfo.isLinux or mozinfo.isMac):
        try:
            
            llvmsym = os.path.join(xrePath, "llvm-symbolizer")
            if os.path.isfile(llvmsym):
                env["ASAN_SYMBOLIZER_PATH"] = llvmsym
                log.info("INFO | runtests.py | ASan using symbolizer at %s"
                         % llvmsym)
            else:
                log.info("TEST-UNEXPECTED-FAIL | runtests.py | Failed to find"
                         " ASan symbolizer at %s" % llvmsym)

            
            
            totalMemory = int(os.popen("free").readlines()[1].split()[1])

            
            
            
            message = "INFO | runtests.py | ASan running in %s configuration"
            asanOptions = []
            if totalMemory <= 1024 * 1024 * 4:
                message = message % 'low-memory'
                asanOptions = [
                    'quarantine_size=50331648', 'malloc_context_size=5']
            else:
                message = message % 'default memory'

            if lsanPath:
                log.info("LSan enabled.")
                asanOptions.append('detect_leaks=1')
                lsanOptions = ["exitcode=0"]
                suppressionsFile = os.path.join(
                    lsanPath, 'lsan_suppressions.txt')
                if os.path.exists(suppressionsFile):
                    log.info("LSan using suppression file " + suppressionsFile)
                    lsanOptions.append("suppressions=" + suppressionsFile)
                else:
                    log.info("WARNING | runtests.py | LSan suppressions file"
                             " does not exist! " + suppressionsFile)
                env["LSAN_OPTIONS"] = ':'.join(lsanOptions)
                
                env['MOZ_CC_RUN_DURING_SHUTDOWN'] = '1'

            if len(asanOptions):
                env['ASAN_OPTIONS'] = ':'.join(asanOptions)

        except OSError, err:
            log.info("Failed determine available memory, disabling ASan"
                     " low-memory configuration: %s" % err.strerror)
        except:
            log.info("Failed determine available memory, disabling ASan"
                     " low-memory configuration")
        else:
            log.info(message)

    tsan = bool(mozinfo.info.get("tsan"))
    if tsan and mozinfo.isLinux:
        
        llvmsym = os.path.join(xrePath, "llvm-symbolizer")
        if os.path.isfile(llvmsym):
            env["TSAN_OPTIONS"] = "external_symbolizer_path=%s" % llvmsym
            log.info("INFO | runtests.py | TSan using symbolizer at %s"
                     % llvmsym)
        else:
            log.info("TEST-UNEXPECTED-FAIL | runtests.py | Failed to find TSan"
                     " symbolizer at %s" % llvmsym)

    return env
