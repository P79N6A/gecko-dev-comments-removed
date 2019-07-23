




































'''Utility methods to be used by python build infrastructure.
'''

import os
import errno
import sys
import time
import stat

class LockFile(object):
  '''LockFile is used by the lockFile method to hold the lock.

  This object should not be used directly, but only through
  the lockFile method below.
  '''
  def __init__(self, lockfile):
    self.lockfile = lockfile
  def __del__(self):
    os.remove(self.lockfile)


def lockFile(lockfile, max_wait = 600):
  '''Create and hold a lockfile of the given name, with the given timeout.

  To release the lock, delete the returned object.
  '''
  while True:
    try:
      fd = os.open(lockfile, os.O_EXCL | os.O_RDWR | os.O_CREAT)
      
      break
    except OSError, e:
      if e.errno != errno.EEXIST:
        
        raise
  
    try:
      
      
      f = open(lockfile, "r")
      s = os.stat(lockfile)
    except EnvironmentError, e:
      if e.errno != errno.ENOENT:
        sys.exit("%s exists but stat() failed: %s" %
                 (lockfile, e.strerror))
      
      
      continue
  
    
    
    now = int(time.time())
    if now - s[stat.ST_MTIME] > max_wait:
      pid = f.readline()
      sys.exit("%s has been locked for more than " \
               "%d seconds (PID %s)" % (lockfile, max_wait,
                                        pid))
  
    
    f.close()
    time.sleep(1)
  
  
  
  
  f = os.fdopen(fd, "w")
  f.write("%d\n" % os.getpid())
  f.close()
  return LockFile(lockfile)

class pushback_iter(object):
  '''Utility iterator that can deal with pushed back elements.

  This behaves like a regular iterable, just that you can call
    iter.pushback(item)
  to get the givem item as next item in the iteration.
  '''
  def __init__(self, iterable):
    self.it = iter(iterable)
    self.pushed_back = []

  def __iter__(self):
    return self

  def __nonzero__(self):
    if self.pushed_back:
      return True

    try:
      self.pushed_back.insert(0, self.it.next())
    except StopIteration:
      return False
    else:
      return True

  def next(self):
    if self.pushed_back:
      return self.pushed_back.pop()
    return self.it.next()

  def pushback(self, item):
    self.pushed_back.append(item)
