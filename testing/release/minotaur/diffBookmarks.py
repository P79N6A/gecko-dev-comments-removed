

































from optparse import OptionParser
from checkBookmarks import bookmarkParser
from logAppender import LogAppender, stderrCatcher
import sys


def main(left, right, log):
  
  lw = LogAppender(log)
  
  sys.stderr = stderrCatcher(lw)

  
  leftParser = bookmarkParser()
  leftParser.parseFile(left)

  
  rightParser = bookmarkParser()
  rightParser.parseFile(right)

  
  
  leftList = leftParser.getList()
  rightList = rightParser.getList()

  if len(leftList) <> len(rightList):
    lw.writeLog("Bookmarks lists are not the same length!")
    raise SystemExit("Bookmark lists not same length, test fails")

  for lentry, rentry in zip(leftList, rightList):
    if lentry <> rentry:
      lw.writeLog("Error found entries that do not match")
      lw.writeLog("Left side: " + lentry[0] + lentry[1])
      lw.writeLog("Right side: " + rentry[0] + rentry[1])
      raise SystemExit("Bookmark entries do not match, test fails")

if __name__ == "__main__":
  parser = OptionParser()
  parser.add_option("-l", "--leftFile", dest="left",
                   help="Bookmarks HTML file 1", metavar="LEFT_FILE")
  parser.add_option("-r", "--rightFile", dest="right",
                    help="Bookmarks HTML file 2", metavar="RIGHT_FILE")
  parser.add_option("-f", "--LogFile", dest="log",
                    help="The file where the log output should go",
                    metavar="LOGFILE")
  (options, args) = parser.parse_args()

  
  main(options.left, options.right, options.log)
