





































class LogAppender:
  def __init__(self, file):
    self.logFile = open(file, "a")
  def writeLog(self, str):
    self.logFile.write(str + "\n")
    return str
  def closeFile(self):
      self.logFile.write("\n---Normal Close---\n")
