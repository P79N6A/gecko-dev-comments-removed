












































from optparse import OptionParser
import os
import os.path
import sys
import shutil

def nsinstall(argv):
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
  p.add_option('-l', action="store_true",
               help="Create link (ignored)")
  p.add_option('-L', action="store", metavar="linkprefix",
               help="Link prefix (ignored)")

  
  
  def BadArg(option, opt, value, parser):
    parser.error('option not supported: %s' % opt)
    
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
      sys.stderr.write('nsinstall: ' + options.m + ' is not a valid mode\n')
      return 1

  
  if options.D:
    if len(args) != 1:
      return 1
    if os.path.exists(args[0]):
      if not os.path.isdir(args[0]):
        sys.stderr.write('nsinstall: ' + args[0] + ' is not a directory\n')
        sys.exit(1)
      if options.m:
        os.chmod(args[0], options.m)
      return 0
    if options.m:
      os.makedirs(args[0], options.m)
    else:
      os.makedirs(args[0])
    return 0

  
  if len(args) < 2:
    p.error('not enough arguments')

  def copy_all_entries(entries, target):
    for e in entries:
      dest = os.path.join(target,
                          os.path.basename(os.path.normpath(e)))
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
      elif options.t:
        shutil.copy2(srcpath, targetpath)
      else:
        shutil.copy(srcpath, targetpath)

  
  target = args.pop()
  
  if not os.path.isdir(target):
    os.makedirs(target)

  copy_all_entries(args, target)
  return 0

if __name__ == '__main__':
  sys.exit(nsinstall(sys.argv[1:]))
