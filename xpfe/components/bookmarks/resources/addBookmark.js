































































































var gFld_Name   = null;
var gFld_URL    = null; 
var gFld_ShortcutURL = null; 
var gFolderTree = null;
var gCB_AddGroup = null;

var gBookmarkCharset = null;

const kRDFSContractID = "@mozilla.org/rdf/rdf-service;1";
const kRDFSIID = Components.interfaces.nsIRDFService;
const kRDF = Components.classes[kRDFSContractID].getService(kRDFSIID);

var gSelectItemObserver = null;

var gCreateInFolder = "NC:NewBookmarkFolder";

function Startup()
{
  gFld_Name = document.getElementById("name");
  gFld_URL = document.getElementById("url");
  gFld_ShortcutURL = document.getElementById("shortcutUrl");
  gCB_AddGroup = document.getElementById("addgroup");
  var bookmarkView = document.getElementById("bookmarks-view");

  var shouldSetOKButton = true;
  var dialogElement = document.documentElement;
  if ("arguments" in window) {
    var ind;
    var folderItem = null;
    var arg;
    if (window.arguments.length < 5)
      arg = null;
    else
      arg = window.arguments[4];
    switch (arg) {
    case "selectFolder":
      
      document.getElementById("bookmarknamegrid").hidden = true;
      document.getElementById("showaddgroup").hidden = true;
      document.getElementById("destinationSeparator").hidden = true;
      document.getElementById("nameseparator").hidden = true;
      document.title = dialogElement.getAttribute("selectFolderTitle");
      shouldSetOKButton = false;
      if (window.arguments[2])
        folderItem = RDF.GetResource(window.arguments[2]);
      if (folderItem) {
        ind = bookmarkView.treeBuilder.getIndexOfResource(folderItem);
        bookmarkView.tree.view.selection.select(ind);
      }
      break;
    case "newBookmark":
      document.getElementById("showaddgroup").hidden = true;
      document.getElementById("destinationSeparator").hidden = true;
      document.getElementById("folderbox").hidden = true;
      setupFields();
      if (window.arguments[2])
        gCreateInFolder = window.arguments[2];
      break;
    case "addGroup":
      setupFields();
      break;
    case "addGroup,group":
      gCB_AddGroup.checked = true;
      setupFields();
      toggleGroup();
      break;
    default:
      
      document.getElementById("showaddgroup").hidden = true;
      setupFields();
      if (window.arguments[2]) {
        gCreateInFolder = window.arguments[2];
        folderItem = RDF.GetResource(gCreateInFolder);
        if (folderItem) {
          ind = bookmarkView.treeBuilder.getIndexOfResource(folderItem);
          bookmarkView.tree.view.selection.select(ind);
        }
      }
    }
  }

  if ((arg != "newBookmark") && (bookmarkView.currentIndex == -1)) {
    var folder = BookmarksUtils.getNewBookmarkFolder();
    ind = bookmarkView.treeBuilder.getIndexOfResource(folder);
    if (ind != -1)
      bookmarkView.tree.view.selection.select(ind);
    else
      bookmarkView.tree.view.selection.select(0);
  }

  if (shouldSetOKButton)
    onFieldInput();
  if (document.getElementById("bookmarknamegrid").hidden) {
    bookmarkView.tree.focus();
  } else {
    gFld_Name.select();
    gFld_Name.focus();
  }

  
  dialogElement.removeAttribute("height");
  dialogElement.removeAttribute("width");
  sizeToContent();
}

function setupFields()
{
  
  gFld_Name.value = window.arguments[0] || "";
  gFld_URL.value = window.arguments[1] || "";
  onFieldInput();
  gFld_Name.select();
  gFld_Name.focus();
  gBookmarkCharset = window.arguments[3] || null;
}

function onFieldInput()
{
  const ok = document.documentElement.getButton("accept");
  ok.disabled = gFld_URL.value == "" && !gCB_AddGroup.checked ||
                gFld_Name.value == "";
}    

function onOK()
{
  if (!document.getElementById("folderbox").hidden) {
    var bookmarkView = document.getElementById("bookmarks-view");
    var currentIndex = bookmarkView.currentIndex;
    if (currentIndex != -1)
      gCreateInFolder = bookmarkView.treeBuilder.getResourceAtIndex(currentIndex).Value;
  }
  
  
  if (window.arguments.length > 4 && window.arguments[4] == "selectFolder") {
    window.arguments[5].target = BookmarksUtils.getTargetFromFolder(bookmarkView.treeBuilder.getResourceAtIndex(currentIndex));
  }
  else {
    

    const kBMDS = kRDF.GetDataSource("rdf:bookmarks");
    const kBMSContractID = "@mozilla.org/browser/bookmarks-service;1";
    const kBMSIID = Components.interfaces.nsIBookmarksService;
    const kBMS = Components.classes[kBMSContractID].getService(kBMSIID);
    var rFolder = kRDF.GetResource(gCreateInFolder, true);
    const kRDFCContractID = "@mozilla.org/rdf/container;1";
    const kRDFIID = Components.interfaces.nsIRDFContainer;
    const kRDFC = Components.classes[kRDFCContractID].getService(kRDFIID);
    try {
      kRDFC.Init(kBMDS, rFolder);
    }
    catch (e) {
      
      rFolder = kRDF.GetResource("NC:BookmarksRoot", true);
      kRDFC.Init(kBMDS, rFolder);
    }

    var url;
    if (gCB_AddGroup.checked) {
      const group = kBMS.createGroupInContainer(gFld_Name.value, rFolder, -1);
      const groups = window.arguments[5];
      for (var i = 0; i < groups.length; ++i) {
        url = getNormalizedURL(groups[i].url);
        kBMS.createBookmarkInContainer(groups[i].name, url, null, null,
                                       groups[i].charset, group, -1);
      }
    } else if (gFld_URL.value) {
      url = getNormalizedURL(gFld_URL.value);
      var newBookmark = kBMS.createBookmarkInContainer(gFld_Name.value, url, gFld_ShortcutURL.value, null, gBookmarkCharset, rFolder, -1);
      if (window.arguments.length > 4 && window.arguments[4] == "newBookmark") {
        window.arguments[5].newBookmark = newBookmark;
      }
    }
  }
}

function getNormalizedURL(url)
{
  
  
  try {
    const kLF = Components.classes["@mozilla.org/file/local;1"]
                          .createInstance(Components.interfaces.nsILocalFile);
    kLF.initWithPath(url);
    if (kLF.exists()) {
      var ioService = Components.classes["@mozilla.org/network/io-service;1"]
                                .getService(Components.interfaces.nsIIOService);
      var fileHandler = ioService.getProtocolHandler("file")
                                 .QueryInterface(Components.interfaces.nsIFileProtocolHandler);

      url = fileHandler.getURLSpecFromFile(kLF);
    }
  }
  catch (e) {
  }

  return url;
}

var gBookmarksShell = null;
function createNewFolder()
{
  var bookmarksView = document.getElementById("bookmarks-view");
  var resource = bookmarksView.treeBuilder.getResourceAtIndex(bookmarksView.currentIndex);
  var target = BookmarksUtils.getTargetFromFolder(resource);
  BookmarksCommand.createNewFolder(target);
}

var gOldNameValue = "";
var gOldURLValue = "";
var gOldShortcutURLValue = "";

function toggleGroup()
{
  
  var temp = gOldNameValue;
  gOldNameValue = gFld_Name.value;
  gFld_Name.value = temp;

  
  temp = gOldURLValue;
  gOldURLValue = gFld_URL.value;
  gFld_URL.value = temp;
  gFld_URL.disabled = gCB_AddGroup.checked;

  
  temp = gOldShortcutURLValue;
  gOldShortcutURLValue = gFld_ShortcutURL.value;
  gFld_ShortcutURL.value = temp;
  gFld_ShortcutURL.disabled = gCB_AddGroup.checked;

  gFld_Name.select();
  gFld_Name.focus();
  onFieldInput();
}

function persistTreeSize()
{
  if (!document.getElementById("folderbox").hidden) {
    var bookmarkView = document.getElementById("bookmarks-view");
    bookmarkView.setAttribute("height", bookmarkView.boxObject.height);
    document.persist("bookmarks-view", "height");
    bookmarkView.setAttribute("width", bookmarkView.boxObject.width);
    document.persist("bookmarks-view", "width");
  }
}
