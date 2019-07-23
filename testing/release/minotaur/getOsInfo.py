

































import re
from optparse import OptionParser
import platform

cygwinmatch = re.compile(".*cygwin.*", re.I)

def getPlatform():
  
  
  if platform.system() == "Microsoft" or cygwinmatch.search(platform.system()):
    print "Windows"
  else:
    print platform.system()

def getFxName(os):
  if os == "Darwin":
    print "firefox-bin"
  elif os == "Linux":
    print "firefox"
  else:
    print "firefox.exe"

def main(os, fxname):
  
  
  
  

  retval = ""

  if not os:
    getPlatform()
  elif os and fxname:
    getFxName(os)
  else:
    raise SystemExit("Invalid Command use getOsInfo --h for help")

if __name__ == "__main__":
  parser = OptionParser()
  parser.add_option("-o", "--os", dest="os",
                   help="OS identifer - either Darwin, Linux, or Windows can be\
                        obtained by calling without any params", metavar="OS")
  parser.add_option("-f", "--firefoxName", action="store_true", dest="fxname", default=False,
                    help="Firefox executable name on this platform requires OS")
  (options, args) = parser.parse_args()

  
  main(options.os, options.fxname)
