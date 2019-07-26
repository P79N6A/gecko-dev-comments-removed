



__all__ = ['check_for_crashes',
           'check_for_java_exception']

import glob
import os
import re
import shutil
import subprocess
import sys
import tempfile
import urllib2
import zipfile

import mozfile
import mozlog


def check_for_crashes(dump_directory, symbols_path,
                      stackwalk_binary=None,
                      dump_save_path=None,
                      test_name=None,
                      quiet=False):
    """
    Print a stack trace for minidump files left behind by a crashing program.

    `dump_directory` will be searched for minidump files. Any minidump files found will
    have `stackwalk_binary` executed on them, with `symbols_path` passed as an extra
    argument.

    `stackwalk_binary` should be a path to the minidump_stackwalk binary.
    If `stackwalk_binary` is not set, the MINIDUMP_STACKWALK environment variable
    will be checked and its value used if it is not empty.

    `symbols_path` should be a path to a directory containing symbols to use for
    dump processing. This can either be a path to a directory containing Breakpad-format
    symbols, or a URL to a zip file containing a set of symbols.

    If `dump_save_path` is set, it should be a path to a directory in which to copy minidump
    files for safekeeping after a stack trace has been printed. If not set, the environment
    variable MINIDUMP_SAVE_PATH will be checked and its value used if it is not empty.

    If `test_name` is set it will be used as the test name in log output. If not set the
    filename of the calling function will be used.

    If `quiet` is set, no PROCESS-CRASH message will be printed to stdout if a
    crash is detected.

    Returns True if any minidumps were found, False otherwise.
    """
    dumps = glob.glob(os.path.join(dump_directory, '*.dmp'))
    if not dumps:
        return False

    if stackwalk_binary is None:
        stackwalk_binary = os.environ.get('MINIDUMP_STACKWALK', None)

    
    if test_name is None:
        try:
            test_name = os.path.basename(sys._getframe(1).f_code.co_filename)
        except:
            test_name = "unknown"

    try:
        log = mozlog.getLogger('mozcrash')
        remove_symbols = False
        
        
        if symbols_path and mozfile.is_url(symbols_path):
            log.info("Downloading symbols from: %s", symbols_path)
            remove_symbols = True
            
            data = urllib2.urlopen(symbols_path)
            symbols_file = tempfile.TemporaryFile()
            symbols_file.write(data.read())
            
            
            symbols_path = tempfile.mkdtemp()
            zfile = zipfile.ZipFile(symbols_file, 'r')
            mozfile.extract_zip(zfile, symbols_path)
            zfile.close()

        for d in dumps:
            extra = os.path.splitext(d)[0] + '.extra'

            stackwalk_output = []
            stackwalk_output.append("Crash dump filename: " + d)
            top_frame = None
            if symbols_path and stackwalk_binary and os.path.exists(stackwalk_binary):
                
                p = subprocess.Popen([stackwalk_binary, d, symbols_path],
                                     stdout=subprocess.PIPE,
                                     stderr=subprocess.PIPE)
                (out, err) = p.communicate()
                if len(out) > 3:
                    
                    
                    stackwalk_output.append(out)
                    
                    
                    
                    
                    
                    
                    lines = out.splitlines()
                    for i, line in enumerate(lines):
                        if "(crashed)" in line:
                            match = re.search(r"^ 0  (?:.*!)?(?:void )?([^\[]+)", lines[i+1])
                            if match:
                                top_frame = "@ %s" % match.group(1).strip()
                            break
                else:
                    stackwalk_output.append("stderr from minidump_stackwalk:")
                    stackwalk_output.append(err)
                if p.returncode != 0:
                    stackwalk_output.append("minidump_stackwalk exited with return code %d" % p.returncode)
            else:
                if not symbols_path:
                    stackwalk_output.append("No symbols path given, can't process dump.")
                if not stackwalk_binary:
                    stackwalk_output.append("MINIDUMP_STACKWALK not set, can't process dump.")
                elif stackwalk_binary and not os.path.exists(stackwalk_binary):
                    stackwalk_output.append("MINIDUMP_STACKWALK binary not found: %s" % stackwalk_binary)
            if not top_frame:
                top_frame = "Unknown top frame"
            if not quiet:
                print "PROCESS-CRASH | %s | application crashed [%s]" % (test_name, top_frame)
                print '\n'.join(stackwalk_output)
            if dump_save_path is None:
                dump_save_path = os.environ.get('MINIDUMP_SAVE_PATH', None)
            if dump_save_path:
                
                
                if os.path.isfile(dump_save_path):
                    os.unlink(dump_save_path)
                if not os.path.isdir(dump_save_path):
                    try:
                        os.makedirs(dump_save_path)
                    except OSError:
                        pass

                shutil.move(d, dump_save_path)
                log.info("Saved minidump as %s",
                         os.path.join(dump_save_path, os.path.basename(d)))

                if os.path.isfile(extra):
                    shutil.move(extra, dump_save_path)
                    log.info("Saved app info as %s",
                             os.path.join(dump_save_path, os.path.basename(extra)))
            else:
                mozfile.remove(d)
                mozfile.remove(extra)
    finally:
        if remove_symbols:
            mozfile.remove(symbols_path)

    return True


def check_for_java_exception(logcat):
    """
    Print a summary of a fatal Java exception, if present in the provided
    logcat output.

    Example:
    PROCESS-CRASH | java-exception | java.lang.NullPointerException at org.mozilla.gecko.GeckoApp$21.run(GeckoApp.java:1833)

    `logcat` should be a list of strings.

    Returns True if a fatal Java exception was found, False otherwise.
    """
    found_exception = False

    for i, line in enumerate(logcat):
        
        
        
        
        
        
        if "REPORTING UNCAUGHT EXCEPTION" in line or "FATAL EXCEPTION" in line:
            
            
            found_exception = True
            if len(logcat) >= i + 3:
                logre = re.compile(r".*\): \t?(.*)")
                m = logre.search(logcat[i+1])
                if m and m.group(1):
                    exception_type = m.group(1)
                m = logre.search(logcat[i+2])
                if m and m.group(1):
                    exception_location = m.group(1)
                print "PROCESS-CRASH | java-exception | %s %s" % (exception_type, exception_location)
            else:
                print "Automation Error: Logcat is truncated!"
            break

    return found_exception
