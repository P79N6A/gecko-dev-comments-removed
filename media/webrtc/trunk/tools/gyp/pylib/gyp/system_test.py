





import os
import tempfile
import shutil
import subprocess


def TestCommands(commands, files={}, env={}):
  """Run commands in a temporary directory, returning true if they all succeed.
  Return false on failures or if any commands produce output.

  Arguments:
    commands: an array of shell-interpretable commands, e.g. ['ls -l', 'pwd']
              each will be expanded with Python %-expansion using env first.
    files: a dictionary mapping filename to contents;
           files will be created in the temporary directory before running
           the command.
    env: a dictionary of strings to expand commands with.
  """
  tempdir = tempfile.mkdtemp()
  try:
    for name, contents in files.items():
      f = open(os.path.join(tempdir, name), 'wb')
      f.write(contents)
      f.close()
    for command in commands:
      proc = subprocess.Popen(command % env, shell=True,
                              stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT,
                              cwd=tempdir)
      output = proc.communicate()[0]
      if proc.returncode != 0 or output:
        return False
    return True
  finally:
    shutil.rmtree(tempdir)
  return False


def TestArSupportsT(ar_command='ar', cc_command='cc'):
  """Test whether 'ar' supports the 'T' flag."""
  return TestCommands(['%(cc)s -c test.c',
                       '%(ar)s crsT test.a test.o',
                       '%(cc)s test.a'],
                      files={'test.c': 'int main(){}'},
                      env={'ar': ar_command, 'cc': cc_command})


def main():
  
  def RunTest(description, function, **kwargs):
    print "Testing " + description + ':',
    if function(**kwargs):
      print 'ok'
    else:
      print 'fail'
  RunTest("ar 'T' flag", TestArSupportsT)
  RunTest("ar 'T' flag with ccache", TestArSupportsT, cc_command='ccache cc')
  return 0


if __name__ == '__main__':
  sys.exit(main())
