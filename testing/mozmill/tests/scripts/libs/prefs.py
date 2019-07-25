



































import re
import sys

class UpdateChannel(object):

    
    channels = ["betatest", "beta", "nightly", "releasetest", "release"]

    
    def isValidChannel(self, channel):
        try:
            self.channels.index(channel);
            return True
        except:
            return False

    
    def getChannel(self):
        try:
            file = open(self.getPrefFolder(), "r")
        except IOError, e:
            raise e
        else:
            content = file.read()
            file.close()

            result = re.search(r"(" + '|'.join(self.channels) + ")", content)
            return result.group(0)

    
    def setChannel(self, channel):
        print "Setting update channel to '%s'..." % channel

        if not self.isValidChannel(channel):
            raise Exception("%s is not a valid update channel" % channel)

        try:
            file = open(self.getPrefFolder(), "r")
        except IOError, e:
            raise e
        else:
            
            content = file.read()
            file.close()

            
            result = re.sub(r"(" + '|'.join(self.channels) + ")",
                            channel, content)

            try:
                file = open(self.getPrefFolder(), "w")
            except IOError, e:
                raise e
            else:
                file.write(result)
                file.close()

                
                if channel != self.getChannel():
                    raise Exception("Update channel wasn't set correctly.")

    
    def getFolder(self):
        return self.folder

    
    def setFolder(self, folder):
        self.folder = folder

    
    def getPrefFolder(self):
        if sys.platform == "darwin":
            return self.folder + "/Contents/MacOS/defaults/pref/channel-prefs.js"
        elif sys.platform == "linux2":
            return self.folder + "/defaults/pref/channel-prefs.js"
        elif sys.platform == "win32":
            return self.folder  + "\\defaults\\pref\\channel-prefs.js"
