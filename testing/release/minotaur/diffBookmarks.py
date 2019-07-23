

































from optparse import OptionParser
import difflib
from checkBookmarks import bookmarkParser
from logAppender import LogAppender, stderrCatcher
import sys

DIFFBKMK_DBG = False

def debug(s):
  if DIFFBKMK_DBG:
    print s


def main(left, right, log):
  
  lw = LogAppender(log)
  
  sys.stderr = stderrCatcher(lw)

  
  leftParser = bookmarkParser(isDebug=DIFFBKMK_DBG)
  leftParser.parseFile(left)

  
  rightParser = bookmarkParser(isDebug=DIFFBKMK_DBG)
  rightParser.parseFile(right)

  
  
  leftList = leftParser.getList()
  rightList = rightParser.getList()

  
  if len(leftList) == 0:
    lw.writeLog("**** BOOKMARKS REFERENCE FILE IS MISSING ****")
    raise SystemExit("**** BOOKMARKS REFERENCE FILE IS MISSING ****")
  elif len(rightList) == 0:
    lw.writeLog("**** BOOKMARKS DATA FOR BUILD UNDER TEST IS MISSING ****")
    raise SystemExit("**** BOOKMARKS DATA FOR BUILD UNDER TEST IS MISSING ****")

  leftlines = []
  for lentry in leftList:
    leftlines.append(lentry[0] + lentry[1] + lentry[2] + lentry[3] + lentry[4] + "\n")

  rightlines = []
  for rentry in rightList:
    rightlines.append(rentry[0] + rentry[1] + rentry[2] + rentry[3] + rentry[4] + "\n")
  
  
  
  
  
  diff = difflib.unified_diff(leftlines, rightlines)
  lw.writelines(diff)

if __name__ == "__main__":
  parser = OptionParser()
  parser.add_option("-l", "--leftFile", dest="left",
                   help="Bookmarks HTML file 1", metavar="LEFT_FILE")
  parser.add_option("-r", "--rightFile", dest="right",
                    help="Bookmarks HTML file 2", metavar="RIGHT_FILE")
  parser.add_option("-f", "--LogFile", dest="log",
                    help="The file where the log output should go",
                    metavar="LOGFILE")
  parser.add_option("-d", "--Debug", dest="isDebug", default=False,
                    help="Turn on debug output by specifying -d true")

  (options, args) = parser.parse_args()

  if options.isDebug:
    DIFFBKMK_DBG = True

  
  main(options.left, options.right, options.log)
