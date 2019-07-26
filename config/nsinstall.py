










from __future__ import print_function
from optparse import OptionParser
import os
import os.path
import sys
import shutil
import stat

def _nsinstall_internal(argv):
  usage = "usage: %prog [options] arg1 [arg2 ...] target-directory"
  p = OptionParser(usage=usage)

  p.add_option('-D', action="store_true",
               help="Create a single directory only")
  p.add_option('-t', action="store_true",
               help="Preserve time stamp")
  p.add_option('-m', action="store",
               help="Set mode", metavar="mode")
  p.add_option('-d', action="store_true",
               help="Create directories in target")
  p.add_option('-R', action="store_true",
               help="Use relative symbolic links (ignored)")
  p.add_option('-L', action="store", metavar="linkprefix",
               help="Link prefix (ignored)")
  p.add_option('-X', action="append", metavar="file",
               help="Ignore a file when installing a directory recursively.")

  
  
  def BadArg(option, opt, value, parser):
    parser.error('option not supported: {0}'.format(opt))
    
  p.add_option('-C', action="callback", metavar="CWD",
               callback=BadArg,
               help="NOT SUPPORTED")
  p.add_option('-o', action="callback", callback=BadArg,
               help="Set owner (NOT SUPPORTED)", metavar="owner")
  p.add_option('-g', action="callback", callback=BadArg,
               help="Set group (NOT SUPPORTED)", metavar="group")

  (options, args) = p.parse_args(argv)

  if options.m:
    
    try:
      options.m = int(options.m, 8)
    except:
      sys.stderr.write('nsinstall: {0} is not a valid mode\n'
                       .format(options.m))
      return 1

  
  def maybe_create_dir(dir, mode, try_again):
    dir = os.path.abspath(dir)
    if os.path.exists(dir):
      if not os.path.isdir(dir):
        print('nsinstall: {0} is not a directory'.format(dir), file=sys.stderr)
        return 1
      if mode:
        os.chmod(dir, mode)
      return 0

    try:
      if mode:
        os.makedirs(dir, mode)
      else:
        os.makedirs(dir)
    except Exception as e:
      
      if try_again:
        return maybe_create_dir(dir, mode, False)
      print("nsinstall: failed to create directory {0}: {1}".format(dir, e))
      return 1
    else:
      return 0

  if options.X:
    options.X = [os.path.abspath(p) for p in options.X]

  if options.D:
    return maybe_create_dir(args[0], options.m, True)

  
  if len(args) < 2:
    p.error('not enough arguments')

  def copy_all_entries(entries, target):
    for e in entries:
      e = os.path.abspath(e)
      if options.X and e in options.X:
        continue

      dest = os.path.join(target, os.path.basename(e))
      dest = os.path.abspath(dest)
      handleTarget(e, dest)
      if options.m:
        os.chmod(dest, options.m)

  
  if options.d:
    
    def handleTarget(srcpath, targetpath):
      
      os.mkdir(targetpath)
  else:
    
    def handleTarget(srcpath, targetpath):
      if os.path.isdir(srcpath):
        if not os.path.exists(targetpath):
          os.mkdir(targetpath)
        entries = [os.path.join(srcpath, e) for e in os.listdir(srcpath)]
        copy_all_entries(entries, targetpath)
        
        if options.m:
          os.chmod(targetpath, options.m)
      else:
        if os.path.exists(targetpath):
          
          os.chmod(targetpath, stat.S_IWUSR)
          os.remove(targetpath)
        if options.t:
          shutil.copy2(srcpath, targetpath)
        else:
          shutil.copy(srcpath, targetpath)

  
  target = args.pop()
  
  
  rv = maybe_create_dir(target, None, True)
  if rv != 0:
    return rv

  copy_all_entries(args, target)
  return 0


def nsinstall(argv):
  return _nsinstall_internal([unicode(arg, "utf-8") for arg in argv])

if __name__ == '__main__':
  
  
  
  
  
  if sys.platform == "win32":
    import ctypes
    from ctypes import wintypes
    GetCommandLine = ctypes.windll.kernel32.GetCommandLineW
    GetCommandLine.argtypes = []
    GetCommandLine.restype = wintypes.LPWSTR

    CommandLineToArgv = ctypes.windll.shell32.CommandLineToArgvW
    CommandLineToArgv.argtypes = [wintypes.LPWSTR, ctypes.POINTER(ctypes.c_int)]
    CommandLineToArgv.restype = ctypes.POINTER(wintypes.LPWSTR)

    argc = ctypes.c_int(0)
    argv_arr = CommandLineToArgv(GetCommandLine(), ctypes.byref(argc))
    
    argv = argv_arr[1:argc.value]
  else:
    
    if sys.stdin.encoding is not None:
      argv = [unicode(arg, sys.stdin.encoding) for arg in sys.argv]
    else:
      argv = [unicode(arg) for arg in sys.argv]

  sys.exit(_nsinstall_internal(argv[1:]))
