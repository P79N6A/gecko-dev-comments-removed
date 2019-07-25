



































import glob
import os
import shutil
import subprocess
import sys
import tempfile
import time

class Installer(object):

    def install(self, package=None, destination="./"):
        
        if not os.path.isfile(package):
            str = "%s is not valid install package." % (package)
            raise Exception(str)

        destination = os.path.join(destination, "")
        fileName = os.path.basename(package)
        fileExt = os.path.splitext(package)[1]

        
        if sys.platform == "win32":
            
            self.uninstall(destination)

            if fileExt == ".exe":
                print "Installing %s => %s" % (fileName, destination)
                cmdArgs = [package, "-ms", "/D=%s" % destination]
                result = subprocess.call(cmdArgs)
            else:
                raise Exception("File type %s not supported." % fileExt)

        
        elif sys.platform == "linux2":
            
            self.uninstall(destination)

            if fileExt == ".bz2":
                print "Installing %s => %s" % (fileName, destination)
                try:
                    os.mkdir(destination);
                except:
                    pass

                cmdArgs = ["tar", "xjf", package, "-C", destination,
                           "--strip-components=1"]
                result = subprocess.call(cmdArgs)
            else:
                raise Exception("File type %s not supported." % fileExt)

        elif sys.platform == "darwin":
            
            mountpoint = tempfile.mkdtemp()
            print "Mounting %s => %s" % (fileName, mountpoint)
            cmdArgs = ["hdiutil", "attach", package, "-mountpoint", mountpoint]
            result = subprocess.call(cmdArgs)

            if not result:
                
                try:
                    bundle = glob.glob(mountpoint + '/*.app')[0]
                    targetFolder = destination + os.path.basename(bundle)

                    
                    self.uninstall(targetFolder)
                    print "Copying %s to %s" % (bundle, targetFolder)
                    shutil.copytree(bundle, targetFolder)

                    destination = targetFolder
                except Exception, e:
                    if not os.path.exists(targetFolder):
                        print "Failure in copying the application files"
                    raise e
                else:
                    
                    print "Unmounting %s..." % fileName
                    cmdArgs = ["hdiutil", "detach", mountpoint]
                    result = subprocess.call(cmdArgs)

        
        return destination

    def uninstall(self, folder):
        ''' Uninstall the build in the given folder '''

        if sys.platform == "win32":
            
            log_file = "%suninstall\uninstall.log" % folder
            if os.path.exists(log_file):
                try:
                    print "Uninstalling %s" % folder
                    cmdArgs = ["%suninstall\helper.exe" % folder, "/S"]
                    result = subprocess.call(cmdArgs)
            
                    
                    
                    
                    timeout = 20
                    while (os.path.exists(log_file) & (timeout > 0)):
                        time.sleep(1)
                        timeout -= 1
                except Exception, e:
                    pass

        
        
        contents = "Contents/MacOS/" if sys.platform == "darwin" else ""
        if os.path.exists("%s/%sapplication.ini" % (folder, contents)):
            try:
                print "Removing old installation at %s" % (folder)
                shutil.rmtree(folder)

                
                timeout = 20
                while (os.path.exists(folder) & (timeout > 0)):
                    time.sleep(1)
                    timeout -= 1
            except:
                print "Folder '%s' could not be removed" % (folder)
                pass
