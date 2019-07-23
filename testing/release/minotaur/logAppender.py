










































class stderrCatcher:
  def __init__(self, logappender=0):
    self.lf = logappender
  def write(self, stuff):
    if self.lf:
      self.lf.onStdError(stuff)

class LogAppender:
  def __init__(self, file):
    self.logFile = open(file, "a")
  def onStdError(self, str):
    self.logFile.write("STDERR:" + str + "\n")
  def writeLog(self, str):
    self.logFile.write(str + "\n")
    return str
  def writelines(self, str):
    self.logFile.writelines(str)
    self.logFile.write("\n")
  def closeFile(self):
      self.logFile.write("\n---Normal Close---\n")
