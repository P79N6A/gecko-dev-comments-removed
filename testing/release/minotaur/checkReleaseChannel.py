






























import re
from optparse import OptionParser
from logAppender import LogAppender, stderrCatcher
import sys

aus2link = re.compile(".*https:\/\/aus2.mozilla.org.*")

def checkHttpLog(httpLogFile, releaseChannel):
  result = False
  try:
    httpFile = open(httpLogFile, "r")
  except IOError:
    return result, "Http Log File Not Found"
  for line in httpFile:
    if aus2link.match(line):
      
      if line.find(releaseChannel) > 0:
        result = True, ""
        break
  return result, "Unable to find release chanel in HTTP Debug Log"

def main(httpFile, releaseFile, log):
  lf = LogAppender(log)
  sys.stderr = stderrCatcher(lf)
  rf = open(releaseFile, "r")
  
  channel = rf.readline().split("\n")
  result, reason = checkHttpLog(httpFile, channel[0])

  if not result:
    lf.writeLog(reason)
    raise SystemExit("Release Update Channel not found. Test Fails")

if __name__ == "__main__":
  parser = OptionParser()
  parser.add_option("-d", "--DebugHttpLog", dest="httpFile",
                   help="Debug Http Log File", metavar="HTTP_LOG_FILE")
  parser.add_option("-r", "--ReleaseChannelFile", dest="releaseFile",
                    help="Text File with release channel name on first line",
                    metavar="RELEASE_FILE")
  parser.add_option("-l", "--LogFile", dest="log",
                    help="The file where the log output should go",
                    metavar="LOGFILE")
  (options, args) = parser.parse_args()

  
  main(options.httpFile, options.releaseFile, options.log)
