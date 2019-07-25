




import os
import shutil
import subprocess
import sys
import tempfile
import _winreg


VSVARS_PATH = ('C:\\Program Files (x86)\\Microsoft Visual Studio 9.0\\'
               'Common7\\Tools\\vsvars32.bat')


def run_with_vsvars(cmd):
  (fd, filename) = tempfile.mkstemp('.bat', text=True)
  f = os.fdopen(fd, "w")
  f.write('@echo off\n')
  f.write('call "%s"\n' % VSVARS_PATH)
  f.write(cmd + '\n')
  f.close()
  try:
    p = subprocess.Popen([filename], shell=True, stdout=subprocess.PIPE,
        universal_newlines=True)
    out, err = p.communicate()
    return p.returncode, out
  finally:
    os.unlink(filename)


def get_vc_dir():
  rc, out = run_with_vsvars('echo VCINSTALLDIR=%VCINSTALLDIR%')
  for line in out.splitlines():
    if line.startswith('VCINSTALLDIR='):
      return line[len('VCINSTALLDIR='):]
  return None


def main():
  vcdir = os.environ.get('VCINSTALLDIR')
  if not vcdir:
    vcdir = get_vc_dir()
    if not vcdir:
      print 'Couldn\'t get VCINSTALLDIR. Run vsvars32.bat?'
      return 1
    os.environ['PATH'] += (';' + os.path.join(vcdir, 'bin') +
                           ';' + os.path.join(vcdir, '..\\Common7\\IDE'))

  
  os.chdir(os.path.dirname(os.path.abspath(__file__)))

  
  link = os.path.join(vcdir, 'bin', 'link.exe')
  link_backup = os.path.join(vcdir, 'bin', 'link.exe.supalink_orig.exe')
  if not os.path.exists(link):
    print 'link.exe not found at %s' % link
    return 1

  
  
  if not os.path.exists(link_backup):
    try:
      print 'Saving original link.exe...'
      shutil.copyfile(link, link_backup)
    except IOError:
      print (('Wasn\'t able to back up %s to %s. '
              'Not running with Administrator privileges?')
              % (link, link_backup))
      return 1

  
  cpptime = os.path.getmtime('supalink.cpp')
  if (not os.path.exists('supalink.exe') or
      cpptime > os.path.getmtime('supalink.exe')):
    print 'Building supalink.exe...'
    rc, out = run_with_vsvars('cl /nologo /Ox /Zi /W4 /WX /D_UNICODE /DUNICODE'
                              ' /D_CRT_SECURE_NO_WARNINGS /EHsc supalink.cpp'
                              ' /link /out:supalink.exe')
    if rc:
      print out
      print 'Failed to build supalink.exe'
      return 1

  
  exetime = os.path.getmtime('supalink.exe')
  if exetime > os.path.getmtime(link):
    print 'Copying supalink.exe over link.exe...'
    try:
      shutil.copyfile('supalink.exe', link)
    except IOError:
      print ('Wasn\'t able to copy supalink.exe over %s. '
             'Not running with Administrator privileges?' % link)
      return 1

  _winreg.SetValue(_winreg.HKEY_CURRENT_USER,
                   'Software\\Chromium\\supalink_installed',
                   _winreg.REG_SZ,
                   link_backup)

  print 'Linker shim installed. Regenerate via gyp: "gclient runhooks".'
  return 0


if __name__ == '__main__':
  sys.exit(main())
