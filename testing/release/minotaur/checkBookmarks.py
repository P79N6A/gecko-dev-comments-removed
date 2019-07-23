




































import sgmllib
import re

Not_White_Space = re.compile('\S')













class bookmarkParser(sgmllib.SGMLParser):

  def __init__(self, isDebug=0, verbose=0):

    self.marker, self.IN_H3, self.IN_A = range(3)

    sgmllib.SGMLParser.__init__(self, verbose)
    self.isToolbar = False
    self.currentFolder = ""
    self.currentAnchor = ""
    self.currentFeedURL = ""
    self.currentIcon = ""
    self.bookmarkList = []
    self.previousFolders = []
    
    self.isDebug = isDebug

  def debug(self, s):
    if self.isDebug:
      print s

  def parseFile(self, fileName):
    bkmkFile = open(fileName, "r")
    htmlContent = bkmkFile.read()
    self.feed(htmlContent)
    self.close()

  
  def start_h3(self, attributes):
    
    if self.currentFolder:
      self.debug("Adding to previous folders: " + self.currentFolder)
      self.previousFolders.append((self.currentFolder, self.isToolbar))
    self.marker = self.IN_H3
    self.isToolbar = False

    for attr in attributes:
      
      if (attr[0] == 'personal_toolbar_folder' and attr[1] == 'true'):
        self.isToolbar = True
        self.currentFolder = "toolbar"
        self.debug("Found Toolbar")

  
  def start_a(self, attributes):
    self.marker = self.IN_A
    for attr in attributes:
      if (attr[0] == "href"):
        self.debug("Found anchor link: " + attr[1])
        self.currentAnchor = attr[1]
      elif attr[0] == "icon":
        self.currentIcon = attr[1]
      elif attr[0] == "feedurl":
        self.currentFeedURL = attr[1]

  
  def end_a(self):
    self.debug("End A reset")
    self.currentAnchor = ""
    self.currentIcon = ""
    self.currentFeedURL = ""

  
  def handle_data(self, data):
    if (Not_White_Space.match(data)):
      self.debug("in non-whitespace data checking folder name")
      
      if self.marker == self.IN_H3:
        if self.isToolbar:
          self.debug("data:h3 is toolbar")
          self.currentFolder = "toolbar"
        else:
          self.debug("data:h3:not toolbar, currentFolder: " + data)
          self.currentFolder = data

      elif self.marker == self.IN_A:
        
        
        self.debug("data:iN_A: adding following: " + self.currentFolder + "," +
        self.currentAnchor + "," + data)

        self.bookmarkList.append( (" folder=" + self.currentFolder, " title=" + data,
                                   " url=" + self.currentAnchor, " feedURL=" + self.currentFeedURL,
                                   " icon=" + self.currentIcon) )

  
  
  def start_dl(self, attributes):
    return 1

  
  def end_dl(self):
    self.debug("End DL reset")
    
    if len(self.previousFolders) > 0:
      item = self.previousFolders.pop()
      self.debug("Resetting to previous folder: " + item[0])
      self.currentFolder = item[0]
      self.isToolbar = item[1]
    else:
      
      self.isToolbar = False
      self.currentFolder = ""

    
    self.currentAnchor = ""

  def getList(self):
    
    
    
    self.bookmarkList.sort()
    return self.bookmarkList
