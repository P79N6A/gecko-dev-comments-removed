




































import os

def MakeDirectoryContentsWritable(dirname):
  """Recursively makes all the contents of a directory writable.
     Uses os.chmod(filename, 0777), which works on Windows.

  Args:
    dirname: Name of the directory to make contents writable.
  """

  for (root, dirs, files) in os.walk(dirname):
    os.chmod(root, 0777)
    for filename in files:
      os.chmod(os.path.join(root, filename), 0777)
