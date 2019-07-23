




































import urllib
import urlparse
import shutil
import subprocess
import traceback
from optparse import OptionParser
import os

_mozdownload_debug = False

def debug(str):
  if _mozdownload_debug:
    print "DEBUG DOWNLOAD.py: " + str



class mozURLopener(urllib.FancyURLopener):
  def __init__(self, **kwargs):
    if kwargs['user'] != "":
      self.user = kwargs['user']
      self.passwd = kwargs['passwd']
    else:
      self.user = ""
      self.passwd = ""
    self.triedPassword = False

    urllib.FancyURLopener.__init__(self)

  def prompt_user_passwd (self, host, realm):
    debug("mozURLopener: Sending Password")
    self.triedPassword = True
    return(self.user, self.passwd)

  def http_error_401(self, url, fp, errcode, errmsg, headers, data=None):
    debug("mozURLOpener: GOT a 401!!!")
    if not self.triedPassword:
      return urllib.FancyURLopener.http_error_401(self, url, fp, errcode, errmsg, headers, data=None)
    else:
      
      raise IOError, 401
      return None

  def http_error_404(self, url, fp, errcode, errmsg, headers, data=None):
    debug("mozURLOpener: Got a 404!")
    raise IOError, 404
    return None

class MozDownloader:
  def __init__(self, **kwargs):
    assert (kwargs['url'] != "" and kwargs['url'] != None)
    assert (kwargs['dest'] != "" and kwargs['dest'] != None)
    self.url = kwargs['url']
    self.dest = kwargs['dest']
    self.error = 0
    self.user = kwargs['user']
    self.passwd = kwargs['password']

  def download(self):
    
    try:
      opener = mozURLopener(user=self.user, passwd=self.passwd)
      data = opener.open(self.url)
      
      self.ensureDest()
      destfile = open(self.dest, "wb")
      destfile.write(data.read())
      destfile.close()
    except IOError, errcode:
      if str(errcode) == "401":
        print "Download Fails - Invalid username and password"
      elif str(errcode) == "404":
        print "Download Fails - URL does not exist: URL = " + self.url
      else:
        print "Download Fails, IOError, error code: " + str(errcode)
        traceback.print_exc()
    except:
      print "Download Fails, unrecognized error."
      traceback.print_exc()

  def ensureDest(self):
    try:
      
      if self.dest[0] == "~":
        self.dest = os.path.expanduser(self.dest)
      headpath = os.path.split(self.dest)[0]
      try:
        if not os.path.exists(headpath):
          os.makedirs(headpath)
      except:
        print "Error creating directory for download"
    except:
      self.error = 1

  def moveToDest(self):
    try:
      
      parsedUrl = urlparse.urlparse(self.url)
      path = parsedUrl[2]
      pathElements = path.split("/")
      filename = pathElements[len(pathElements) - 1]
      print filename

      
      
      if self.dest[0] == "~":
        self.dest = os.path.expanduser(self.dest)
      headpath = os.path.split(self.dest)[0]
      try:
        if not os.path.exists(headpath):
          os.makedirs(headpath)
      except:
        print "Error creating destination directory"

      
      self.dest = os.path.expandvars(self.dest)
      debug(self.dest)
      shutil.move("./" + urllib.unquote(filename), self.dest)
    except:
      self.error = 1

if __name__ == "__main__":
  parser = OptionParser()
  parser.add_option("-d", "--Destination", dest="dest",
                   help="Destination file to download to",
                   metavar="DEST_FILE")
  parser.add_option("-u", "--URL", dest="url",
                    help="URL to download from", metavar="URL")
  parser.add_option("-n", "--userName", dest="user",
                    help="User name if needed (optional)",
                    metavar="USERNAME")
  parser.add_option("-p", "--Password", dest="password",
                    help="Password for User name (optional)",
                    metavar="PASSWORD")

  (options, args) = parser.parse_args()

  
  mozDwnld = MozDownloader(url=options.url, dest=options.dest, user=options.user,
                           password=options.password)
  mozDwnld.download()
