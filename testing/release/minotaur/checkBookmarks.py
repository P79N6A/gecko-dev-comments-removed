




































import sgmllib
import re

Not_White_Space = re.compile('\S')

def debug(s):
  if (0):
    print s













class bookmarkParser(sgmllib.SGMLParser):

  def __init__(self, verbose=0):

    self.marker, self.IN_H3, self.IN_A = range(3)

    sgmllib.SGMLParser.__init__(self, verbose)
    self.isToolbar = False
    self.currentFolder = ""
    self.currentAnchor = ""
    self.bookmarkList = []

  def parseFile(self, fileName):
    bkmkFile = open(fileName, "r")
    htmlContent = bkmkFile.read()
    self.feed(htmlContent)
    self.close()

  
  def start_h3(self, attributes):
    self.marker = self.IN_H3
    self.isToolbar = False

    for attr in attributes:
      
      if (attr[0] == 'personal_toolbar_folder' and attr[1] == 'true'):
        self.isToolbar = True

 
  def start_a(self, attributes):
    self.marker = self.IN_A
    for attr in attributes:
      if (attr[0] == "href"):
        debug("Found anchor link: " + attr[1])
        self.currentAnchor = attr[1]

  
  def end_a(self):
    debug("End A reset")
    self.currentAnchor = ""

  
  def handle_data(self, data):
    if (Not_White_Space.match(data)):
      debug("in non-whitespace data")
      
      if self.marker == self.IN_H3:
        if self.isToolbar:
          debug("data:h3 is toolbar")
          self.currentFolder = "toolbar"
        else:
          debug("data:h3:not toolbar")
          self.currentFolder = data

      elif self.marker == self.IN_A:
        
        
        debug("data:isA adding following: " + self.currentFolder + "," +
        self.currentAnchor + "," + data)
        self.bookmarkList.append( (self.currentFolder, self.currentAnchor,
                                   data) )

  
  
  def start_dl(self, attributes):
    return 1

  
  def end_dl(self):
    debug("End DL reset")
    self.isToolbar = False
    self.currentFolder = ""
    self.currentAnchor = ""

  def getList(self):
    return self.bookmarkList




def getFolderList(folderName, sortedList):
  fldrList = []
  fldrSet = False
  for s in sortedList:
    if s[0] == folderName:
      fldrList.append(s)
      fldrSet = True
    else:
      if fldrSet:
        
        
          break
      else:
        
        
        continue
  return fldrList

def checkBookmarks(loc, bkmkFile, verifier, log):
  rtn = True
  parser = bookmarkParser()
  parser.parseFile(bkmkFile)

  
  bkmkList = parser.getList()
  bkmkList.sort(lambda x,y: cmp(x[0], y[0]))

  verifiedBkmks = verifier.getElementList("bookmarks")

  
  for vElem in verifiedBkmks:
    fldrList = getFolderList(vElem.getAttribute("folder"), bkmkList)
    for f in fldrList:
      if (vElem.getAttribute("link") == f[1] and
          vElem.getAttribute("title") == f[2]):
        log.writeLog("\n" + vElem.getAttribute("title") + " bookmark PASSES")
      else:
        log.writeLog("\n" + vElem.getAttribute("title") + " bookmark FAILS")
        rtn = False
  return rtn
