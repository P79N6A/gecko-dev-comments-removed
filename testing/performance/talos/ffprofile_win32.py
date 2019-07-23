




































import os

def MakeDirectoryContentsWritable(dirname):
  """Recursively makes all the contents of a directory writable.
     Uses os.chmod(filename, 0777), which works on Windows.

  Args:
    dirname: Name of the directory to make contents writable.
  """

  try:
    for (root, dirs, files) in os.walk(dirname):
      os.chmod(root, 0777)
      for filename in files:
        try:
          os.chmod(os.path.join(root, filename), 0777)
        except OSError, (errno, strerror):
          print 'WARNING: failed to os.chmod(%s): %s : %s' % (os.path.join(root, filename), errno, strerror)
  except OSError, (errno, strerror):
    print 'WARNING: failed to MakeDirectoryContentsWritable: %s : %s' % (errno, strerror)
