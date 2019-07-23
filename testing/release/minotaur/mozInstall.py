




































from optparse import OptionParser
import platform
import subprocess
import re
import time
import string
import os
import shutil

isDMG = re.compile(".*\.dmg")
isTARBZ = re.compile(".*\.tar\.bz")
isTARGZ = re.compile(".*\.tar\.gz")
isZIP = re.compile(".*\.zip")
isEXE = re.compile(".*\.exe")
_mozInstall_debug = False

def debug(s):
  if _mozInstall_debug:
    print "DEBUG: " + s

cygwinmatch = re.compile(".*cygwin.*", re.I)




def getPlatform():
  
  
  if platform.system() == "Microsoft" or cygwinmatch.search(platform.system()):
    return "Windows"
  else:
    return platform.system()




def rmdirRecursive(dir):
  """This is a replacement for shutil.rmtree that works better under
  windows. Thanks to Bear at the OSAF for the code."""
  if not os.path.exists(dir):
      return

  if os.path.islink(dir):
      os.remove(dir)
      return

  
  os.chmod(dir, 0700)

  for name in os.listdir(dir):
      full_name = os.path.join(dir, name)
      
      
      if os.name == 'nt':
          if not os.access(full_name, os.W_OK):
              
              
              
              os.chmod(full_name, 0600)

      if os.path.isdir(full_name):
          rmdirRecursive(full_name)
      else:
          os.chmod(full_name, 0700)
          os.remove(full_name)
  os.rmdir(dir)

class MozUninstaller:
  def __init__(self, **kwargs):
    debug("uninstall constructor")
    assert (kwargs['dest'] != "" and kwargs['dest'] != None)
    assert (kwargs['productName'] != "" and kwargs['productName'] != None)
    assert (kwargs['branch'] != "" and kwargs['dest'] != None)
    self.dest = kwargs['dest']
    self.productName = kwargs['productName']
    self.branch = kwargs['branch']

    
    if not os.path.exists(self.dest):
      return

    if getPlatform() == "Windows":
      try:
        self.doWindowsUninstall()
      except:
        debug("Windows Uninstall threw - not overly urgent or worrisome")
    if os.path.exists(self.dest):
      try:
        os.rmdir(self.dest)
      except OSError:
        
        rmdirRecursive(self.dest)


  def doWindowsUninstall(self):
    debug("do windowsUninstall")
    if self.branch == "1.8.0":
      uninstexe = self.dest + "/uninstall/uninstall.exe"
      uninstini = self.dest + "/uninstall/uninstall.ini"
      debug("uninstexe: " + uninstexe)
      debug("uninstini: " + uninstini)
      if os.path.exists(uninstexe):
        
        debug("modifying uninstall.ini")
        args = "sed -i.bak 's/Run Mode=Normal/Run Mode=Silent/' " + uninstini
        proc = subprocess.Popen(args, shell=True)
        
        proc.wait()
        proc = subprocess.Popen(uninstexe, shell=True)
        proc.wait()
    elif self.branch == "1.8.1" or self.branch == "1.8" or self.branch == "1.9":
      debug("we are in 1.8 uninstall land")
      uninst = self.dest + "/uninstall/uninst.exe"
      helper = self.dest + "/uninstall/helper.exe"
      debug("uninst: " + uninst)
      debug("helper: " + helper)

      if os.path.exists(helper):
        debug("helper exists")
        args = helper + " /S /D=" + os.path.normpath(self.dest)
        debug("running helper with args: " + args)
        proc = subprocess.Popen(args, shell=True)
        proc.wait()
      elif os.path.exists(uninst):
        args = uninst + " /S /D=" + os.path.normpath(self.dest)
        debug("running uninst with args: " + args)
        proc = subprocess.Popen(args, shell=True)
        proc.wait()
      else:
        uninst = self.dest + "/" + self.product + "/uninstall/uninstaller.exe"
        args = uninst + " /S /D=" + os.path.normpath(self.dest)
        debug("running uninstaller with args: " + args)
        proc = subprocess.Popen(args, shell=True)
        proc.wait()
    time.sleep(10)

class MozInstaller:
  def __init__(self, **kwargs):
    debug("install constructor!")
    assert (kwargs['dest'] != "" and kwargs['dest'] != None)
    assert (kwargs['src'] != "" and kwargs['src'] != None)
    assert (kwargs['productName'] != ""  and kwargs['productName'] != None)
    assert (kwargs['branch'] != "" and kwargs['branch'] != None)
    self.src = kwargs['src']
    self.dest = kwargs['dest']
    self.productName = kwargs['productName']
    self.branch = kwargs['branch']
    debug("running uninstall")
    uninstaller = MozUninstaller(dest = self.dest, productName = self.productName,
                                 branch = self.branch)

    if isDMG.match(self.src):
      self.installDmg()
    elif isTARBZ.match(self.src):
      self.installTarBz()
    elif isTARGZ.match(self.src):
      self.installTarGz()
    elif isZIP.match(self.src):
      self.installZip()
    elif isEXE.match(self.src):
      self.installExe()

  
  
  def normalizePath(self, path):
    if path[0] == "~":
      path = os.path.expanduser(path)

    debug("NORMALIZE: path: " + path)

    try:
      if not os.path.exists(path):
        os.makedirs(path)
    except:
      
      print "Error creating destination directory"
    return path

  def installDmg(self):
    
    self.dest = self.normalizePath(self.dest)

    args = "sh installdmg.sh " + self.src + " " + self.dest
    proc = subprocess.Popen(args, shell=True)
    proc.wait()
    

  def installTarBz(self):
    
    self.dest = self.normalizePath(self.dest)
    self.unTar("-jxvf")

  def installTarGz(self):
    
    self.dest = self.normalizePath(self.dest)
    self.unTar("-zxvf")

  def unTar(self, tarArgs):
    args = "tar " + tarArgs + " " + self.src + " -C " + self.dest
    proc = subprocess.Popen(args, shell=True)
    proc.wait()
    

  def installZip(self):
    self.dest = self.normalizePath(self.dest)
    args = "unzip -o -d " + self.dest + " " + self.src
    proc = subprocess.Popen(args, shell=True)
    proc.wait()
    

  def installExe(self):
    debug("running installEXE")
    args = self.src + " "
    if self.branch == "1.8.0":
      args += "-ms -hideBanner -dd " + self.dest
    else:
      debug("running install exe for 1.8.1")
      args += "/S /D=" + os.path.normpath(self.dest)
    
    proc = subprocess.Popen(args)
    proc.wait()
    


if __name__ == "__main__":
  parser = OptionParser()
  parser.add_option("-s", "--Source", dest="src",
                   help="Installation Source File (whatever was downloaded) -\
                         accepts Zip, Exe, Tar.Bz, Tar.Gz, and DMG",
                   metavar="SRC_FILE")
  parser.add_option("-d", "--Destination", dest="dest",
                    help="Directory to install the build into", metavar="DEST")
  parser.add_option("-b", "--Branch", dest="branch",
                    help="Branch the build is from must be one of: 1.8.0|1.8|\
                          1.9", metavar="BRANCH")
  parser.add_option("-p", "--Product", dest="product",
                    help="Product name - optional should be all lowercase if\
                         specified: firefox, thunderbird, etc",
                    metavar="PRODUCT")
  parser.add_option("-o", "--Operation", dest="op",
                    help="The operation you would like the script to perform.\
                         Should be either install (i) or uninstall (u) or delete\
                          (d) to recursively delete the directory specified in dest",
                    metavar="OP")

  (options, args) = parser.parse_args()

  
  if string.upper(options.op) == "INSTALL" or string.upper(options.op) == "I":
    installer = MozInstaller(src = options.src, dest = options.dest,
                             branch = options.branch, productName = options.product)
  elif string.upper(options.op) == "UNINSTALL" or string.upper(options.op) == "U":
    uninstaller = MozUninstaller(dest = options.dest, branch = options.branch,
                                 productName = options.product)
  elif string.upper(options.op) == "DELETE" or string.upper(options.op) == "D":
    rmdirRecursive(options.dest)
