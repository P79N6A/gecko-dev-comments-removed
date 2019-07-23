# -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
#
# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http:
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is mozilla.org code.
#
# The Initial Developer of the Original Code is
# Netscape Communications Corporation.
# Portions created by the Initial Developer are Copyright (C) 2000
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Stuart Parmenter <pavlov@netscape.com>
#   Brian Ryner <bryner@brianryner.com>
#   Jan Varga <varga@ku.sk>
#   Peter Annema <disttsc@bart.nl>
#   Johann Petrak <johann@ai.univie.ac.at>
#   Akkana Peck <akkana@netscape.com>
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****

const nsIFilePicker       = Components.interfaces.nsIFilePicker;
const nsIProperties       = Components.interfaces.nsIProperties;
const NS_DIRECTORYSERVICE_CONTRACTID = "@mozilla.org/file/directory_service;1";
const nsITreeBoxObject = Components.interfaces.nsITreeBoxObject;
const nsIFileView = Components.interfaces.nsIFileView;
const NS_FILEVIEW_CONTRACTID = "@mozilla.org/filepicker/fileview;1";
const nsITreeView = Components.interfaces.nsITreeView;
const nsILocalFile = Components.interfaces.nsILocalFile;
const nsIFile = Components.interfaces.nsIFile;
const NS_LOCAL_FILE_CONTRACTID = "@mozilla.org/file/local;1";
const NS_PROMPTSERVICE_CONTRACTID = "@mozilla.org/embedcomp/prompt-service;1";

var sfile = Components.classes[NS_LOCAL_FILE_CONTRACTID].createInstance(nsILocalFile);
var retvals;
var filePickerMode;
var homeDir;
var treeView;

var textInput;
var okButton;

var gFilePickerBundle;



var gNewDirName = { value: "" };

function filepickerLoad() {
  gFilePickerBundle = document.getElementById("bundle_filepicker");

  textInput = document.getElementById("textInput");
  okButton = document.documentElement.getButton("accept");
  treeView = Components.classes[NS_FILEVIEW_CONTRACTID].createInstance(nsIFileView);

  if (window.arguments) {
    var o = window.arguments[0];
    retvals = o.retvals; 
    const title = o.title;
    filePickerMode = o.mode;
    if (o.displayDirectory) {
      const directory = o.displayDirectory.path;
    }

    const initialText = o.defaultString;
    const filterTitles = o.filters.titles;
    const filterTypes = o.filters.types;
    const numFilters = filterTitles.length;

    document.title = title;

    if (initialText) {
      textInput.value = initialText;
    }
  }

  if (filePickerMode != nsIFilePicker.modeOpen && filePickerMode != nsIFilePicker.modeOpenMultiple) {
    var newDirButton = document.getElementById("newDirButton");
    newDirButton.removeAttribute("hidden");
  }

  if (filePickerMode == nsIFilePicker.modeGetFolder) {
    var textInputLabel = document.getElementById("textInputLabel");
    textInputLabel.value = gFilePickerBundle.getString("dirTextInputLabel");
    textInputLabel.accessKey = gFilePickerBundle.getString("dirTextInputAccesskey");
  }
  
  if ((filePickerMode == nsIFilePicker.modeOpen) ||
      (filePickerMode == nsIFilePicker.modeOpenMultiple) ||
      (filePickerMode == nsIFilePicker.modeSave)) {

    
    var filterPopup = document.createElement("menupopup");

    for (var i = 0; i < numFilters; i++) {
      var menuItem = document.createElement("menuitem");
      if (filterTypes[i] == "..apps")
        menuItem.setAttribute("label", filterTitles[i]);
      else
        menuItem.setAttribute("label", filterTitles[i] + " (" + filterTypes[i] + ")");
      menuItem.setAttribute("filters", filterTypes[i]);
      filterPopup.appendChild(menuItem);
    }

    var filterMenuList = document.getElementById("filterMenuList");
    filterMenuList.appendChild(filterPopup);
    if (numFilters > 0)
      filterMenuList.selectedIndex = 0;
    var filterBox = document.getElementById("filterBox");
    filterBox.removeAttribute("hidden");

    filterMenuList.selectedIndex = o.filterIndex;

    treeView.setFilter(filterTypes[o.filterIndex]);

  } else if (filePickerMode == nsIFilePicker.modeGetFolder) {
    treeView.showOnlyDirectories = true;
  }

  
  if (filePickerMode == nsIFilePicker.modeSave)
    okButton.setAttribute("icon", "save");

  
  handleColumnClick("FilenameColumn");

  try {
    setOKAction();
  } catch (exception) {
    
  }

  
  retvals.buttonStatus = nsIFilePicker.returnCancel;

  var tree = document.getElementById("directoryTree");
  if (filePickerMode == nsIFilePicker.modeOpenMultiple)
    tree.removeAttribute("seltype");

  tree.treeBoxObject.view = treeView;

  
  
  okButton.disabled = filePickerMode != nsIFilePicker.modeGetFolder;

  
  

  setTimeout(setInitialDirectory, 0, directory);
}

function setInitialDirectory(directory)
{
  
  var dirService = Components.classes[NS_DIRECTORYSERVICE_CONTRACTID]
                             .getService(nsIProperties);
  homeDir = dirService.get("Home", Components.interfaces.nsIFile);

  if (directory) {
    sfile.initWithPath(directory);
    if (!sfile.exists() || !sfile.isDirectory())
      directory = false;
  }
  if (!directory) {
    sfile.initWithPath(homeDir.path);
  }

  gotoDirectory(sfile);
}

function onFilterChanged(target)
{
  
  

  setTimeout(changeFilter, 0, target.getAttribute("filters"));
}

function changeFilter(filterTypes)
{
  window.setCursor("wait");
  treeView.setFilter(filterTypes);
  window.setCursor("auto");
}

function showErrorDialog(titleStrName, messageStrName, file)
{
  var errorTitle =
    gFilePickerBundle.getFormattedString(titleStrName, [file.path]);
  var errorMessage =
    gFilePickerBundle.getFormattedString(messageStrName, [file.path]);
  var promptService =
    Components.classes[NS_PROMPTSERVICE_CONTRACTID].getService(Components.interfaces.nsIPromptService);

  promptService.alert(window, errorTitle, errorMessage);
}

function openOnOK()
{
  var dir = treeView.selectedFiles.queryElementAt(0, nsIFile);
  if (dir)
    gotoDirectory(dir);

  return false;
}

function selectOnOK()
{
  var errorTitle, errorMessage, promptService;
  var ret = nsIFilePicker.returnOK;

  var isDir = false;
  var isFile = false;

  var fileList = processPath(textInput.value);

  if (!fileList) { 
    showErrorDialog("errorPathProblemTitle",
                    "errorPathProblemMessage",
                    textInput.value);
    return false;
  }

  var curFileIndex;
  for (curFileIndex = 0; curFileIndex < fileList.length &&
         ret != nsIFilePicker.returnCancel; ++curFileIndex) {
    var file = fileList[curFileIndex].QueryInterface(nsIFile);

    
    
    
    try{
      file.normalize();
    } catch(e) {
      
    }

    var fileExists = file.exists();

    if (!fileExists && (filePickerMode == nsIFilePicker.modeOpen ||
                        filePickerMode == nsIFilePicker.modeOpenMultiple)) {
      showErrorDialog("errorOpenFileDoesntExistTitle",
                      "errorOpenFileDoesntExistMessage",
                      file);
      return false;
    }

    if (!fileExists && filePickerMode == nsIFilePicker.modeGetFolder) {
      showErrorDialog("errorDirDoesntExistTitle",
                      "errorDirDoesntExistMessage",
                      file);
      return false;
    }

    if (fileExists) {
      isDir = file.isDirectory();
      isFile = file.isFile();
    }

    switch(filePickerMode) {
    case nsIFilePicker.modeOpen:
    case nsIFilePicker.modeOpenMultiple:
      if (isFile) {
        if (file.isReadable()) {
          retvals.directory = file.parent.path;
        } else {
          showErrorDialog("errorOpeningFileTitle",
                          "openWithoutPermissionMessage_file",
                          file);
          ret = nsIFilePicker.returnCancel;
        }
      } else if (isDir) {
        if (!sfile.equals(file)) {
          gotoDirectory(file);
        }
        textInput.value = "";
        doEnabling();
        ret = nsIFilePicker.returnCancel;
      }
      break;
    case nsIFilePicker.modeSave:
      if (isFile) { 
        if (!file.isWritable()) {
          showErrorDialog("errorSavingFileTitle",
                          "saveWithoutPermissionMessage_file",
                          file);
          ret = nsIFilePicker.returnCancel;
        } else {
          
          var confirmTitle = gFilePickerBundle.getString("confirmTitle");
          var message =
            gFilePickerBundle.getFormattedString("confirmFileReplacing",
                                                 [file.path]);
          
          promptService = Components.classes[NS_PROMPTSERVICE_CONTRACTID].getService(Components.interfaces.nsIPromptService);
          var rv = promptService.confirm(window, confirmTitle, message);
          if (rv) {
            ret = nsIFilePicker.returnReplace;
            retvals.directory = file.parent.path;
          } else {
            ret = nsIFilePicker.returnCancel;
          }
        }
      } else if (isDir) {
        if (!sfile.equals(file)) {
          gotoDirectory(file);
        }
        textInput.value = "";
        doEnabling();
        ret = nsIFilePicker.returnCancel;
      } else {
        var parent = file.parent;
        if (parent.exists() && parent.isDirectory() && parent.isWritable()) {
          retvals.directory = parent.path;
        } else {
          var oldParent = parent;
          while (!parent.exists()) {
            oldParent = parent;
            parent = parent.parent;
          }
          errorTitle =
            gFilePickerBundle.getFormattedString("errorSavingFileTitle",
                                                 [file.path]);
          if (parent.isFile()) {
            errorMessage =
              gFilePickerBundle.getFormattedString("saveParentIsFileMessage",
                                                   [parent.path, file.path]);
          } else {
            errorMessage =
              gFilePickerBundle.getFormattedString("saveParentDoesntExistMessage",
                                                   [oldParent.path, file.path]);
          }
          if (!parent.isWritable()) {
            errorMessage =
              gFilePickerBundle.getFormattedString("saveWithoutPermissionMessage_dir", [parent.path]);
          }
          promptService = Components.classes[NS_PROMPTSERVICE_CONTRACTID].getService(Components.interfaces.nsIPromptService);
          promptService.alert(window, errorTitle, errorMessage);
          ret = nsIFilePicker.returnCancel;
        }
      }
      break;
    case nsIFilePicker.modeGetFolder:
      if (isDir) {
        retvals.directory = file.parent.path;
      } else { 
        retvals.directory = sfile.path;
      }
      break;
    }
  }

  gFilesEnumerator.mFiles = fileList;

  retvals.files = gFilesEnumerator;
  retvals.buttonStatus = ret;

  var filterMenuList = document.getElementById("filterMenuList");
  retvals.filterIndex = filterMenuList.selectedIndex;
  
  return (ret != nsIFilePicker.returnCancel);
}

var gFilesEnumerator = {
  mFiles: null,
  mIndex: 0,

  hasMoreElements: function()
  {
    return (this.mIndex < this.mFiles.length);
  },
  getNext: function()
  {
    if (this.mIndex >= this.mFiles.length)
      throw Components.results.NS_ERROR_FAILURE;
    return this.mFiles[this.mIndex++];
  }
};

function onCancel()
{
  
  retvals.buttonStatus = nsIFilePicker.returnCancel;
  retvals.file = null;
  retvals.files = null;
  return true;
}

function onDblClick(e) {
  
  if (e.button != 0) return;

  var t = e.originalTarget;
  if (t.localName != "treechildren")
    return;

  openSelectedFile();
}

function openSelectedFile() {
  var fileList = treeView.selectedFiles;
  if (fileList.length == 0)
    return;

  var file = fileList.queryElementAt(0, nsIFile);
  if (file.isDirectory())
    gotoDirectory(file);
  else if (file.isFile())
    document.documentElement.acceptDialog();
}

function onClick(e) {
  var t = e.originalTarget;
  if (t.localName == "treecol")
    handleColumnClick(t.id);
}

function convertColumnIDtoSortType(columnID) {
  var sortKey;
  
  switch (columnID) {
  case "FilenameColumn":
    sortKey = nsIFileView.sortName;
    break;
  case "FileSizeColumn":
    sortKey = nsIFileView.sortSize;
    break;
  case "LastModifiedColumn":
    sortKey = nsIFileView.sortDate;
    break;
  default:
    dump("unsupported sort column: " + columnID + "\n");
    sortKey = 0;
    break;
  }
  
  return sortKey;
}

function handleColumnClick(columnID) {
  var sortType = convertColumnIDtoSortType(columnID);
  var sortOrder = (treeView.sortType == sortType) ? !treeView.reverseSort : false;
  treeView.sort(sortType, sortOrder);
  
  
  var sortedColumn = document.getElementById(columnID);
  if (treeView.reverseSort) {
    sortedColumn.setAttribute("sortDirection", "descending");
  } else {
    sortedColumn.setAttribute("sortDirection", "ascending");
  }
  
  
  var currCol = sortedColumn.parentNode.firstChild;
  while (currCol) {
    if (currCol != sortedColumn && currCol.localName == "treecol")
      currCol.removeAttribute("sortDirection");
    currCol = currCol.nextSibling;
  }
}

function onKeypress(e) {
  if (e.keyCode == 8) 
    goUp();

  
}

function doEnabling() {
  if (filePickerMode != nsIFilePicker.modeGetFolder)
  
  
    okButton.disabled = (textInput.value == "")
}

function onTreeFocus(event) {
  
  onFileSelected(treeView.selectedFiles);
}

function setOKAction(file) {
  var buttonLabel;
  var buttonIcon = "open"; 

  if (file && file.isDirectory()) {
    document.documentElement.setAttribute("ondialogaccept", "return openOnOK();");
    buttonLabel = gFilePickerBundle.getString("openButtonLabel");
  }
  else {
    document.documentElement.setAttribute("ondialogaccept", "return selectOnOK();");
    switch(filePickerMode) {
    case nsIFilePicker.modeGetFolder:
      buttonLabel = gFilePickerBundle.getString("selectFolderButtonLabel");
      break;
    case nsIFilePicker.modeOpen:
    case nsIFilePicker.modeOpenMultiple:
      buttonLabel = gFilePickerBundle.getString("openButtonLabel");
      break;
    case nsIFilePicker.modeSave:
      buttonLabel = gFilePickerBundle.getString("saveButtonLabel");
      buttonIcon = "save";
      break;
    }
  }
  okButton.setAttribute("label", buttonLabel);
  okButton.setAttribute("icon", buttonIcon);
}

function onSelect(event) {
  onFileSelected(treeView.selectedFiles);
}

function onFileSelected( selectedFileList) {
  var validFileSelected = false;
  var invalidSelection = false;
  var file;
  var fileCount = selectedFileList.length;

  for (var index = 0; index < fileCount; ++index) {
    file = selectedFileList.queryElementAt(index, nsIFile);
    if (file) {
      var path = file.leafName;

      if (path) {
        var isDir = file.isDirectory();
        if ((filePickerMode == nsIFilePicker.modeGetFolder) || !isDir) {
          if (!validFileSelected)
            textInput.value = "";
          addToTextFieldValue(path);
        }

        if (isDir && fileCount > 1) {
          
          
          
          invalidSelection = true;
        }

        validFileSelected = true;
      }
    }
  }

  if (validFileSelected) {
    setOKAction(file);
    okButton.disabled = invalidSelection;
  } else if (filePickerMode != nsIFilePicker.modeGetFolder)
    okButton.disabled = (textInput.value == "");
}

function addToTextFieldValue(path)
{
  var newValue = "";

  if (textInput.value == "")
    newValue = path.replace(/\"/g, "\\\"");
  else {
    
    
    if (textInput.value[0] != '"')
      newValue = '"' + textInput.value.replace(/\"/g, "\\\"") + '"';
    else
      newValue = textInput.value;

    newValue = newValue + ' "' + path.replace(/\"/g, "\\\"") + '"';
  }

  textInput.value = newValue;
}

function onTextFieldFocus() {
  setOKAction(null);
  doEnabling();
}

function onDirectoryChanged(target)
{
  var path = target.getAttribute("label");

  var file = Components.classes[NS_LOCAL_FILE_CONTRACTID].createInstance(nsILocalFile);
  file.initWithPath(path);

  if (!sfile.equals(file)) {
    
    

    setTimeout(gotoDirectory, 0, file);
  }
}

function populateAncestorList(directory) {
  var menu = document.getElementById("lookInMenu");

  while (menu.hasChildNodes()) {
    menu.removeChild(menu.firstChild);
  }
  
  var menuItem = document.createElement("menuitem");
  menuItem.setAttribute("label", directory.path);
  menuItem.setAttribute("crop", "start");
  menu.appendChild(menuItem);

  
  var parent = directory.parent;
  while (parent && !parent.equals(directory)) {
    menuItem = document.createElement("menuitem");
    menuItem.setAttribute("label", parent.path);
    menuItem.setAttribute("crop", "start");
    menu.appendChild(menuItem);
    directory = parent;
    parent = directory.parent;
  }
  
  var menuList = document.getElementById("lookInMenuList");
  menuList.selectedIndex = 0;
}

function goUp() {
  try {
    var parent = sfile.parent;
  } catch(ex) { dump("can't get parent directory\n"); }

  if (parent) {
    gotoDirectory(parent);
  }
}

function goHome() {
  gotoDirectory(homeDir);
}

function newDir() {
  var file;
  var promptService =
    Components.classes[NS_PROMPTSERVICE_CONTRACTID].getService(Components.interfaces.nsIPromptService);
  var dialogTitle =
    gFilePickerBundle.getString("promptNewDirTitle");
  var dialogMsg =
    gFilePickerBundle.getString("promptNewDirMessage");
  var ret = promptService.prompt(window, dialogTitle, dialogMsg, gNewDirName, null, {value:0});

  if (ret) {
    file = processPath(gNewDirName.value);
    if (!file) {
      showErrorDialog("errorCreateNewDirTitle",
                      "errorCreateNewDirMessage",
                      file);
      return false;
    }
    
    file = file[0].QueryInterface(nsIFile);
    if (file.exists()) {
      showErrorDialog("errorNewDirDoesExistTitle",
                      "errorNewDirDoesExistMessage",
                      file);
      return false;
    }

    var parent = file.parent;
    if (!(parent.exists() && parent.isDirectory() && parent.isWritable())) {
      var oldParent = parent;
      while (!parent.exists()) {
        oldParent = parent;
        parent = parent.parent;
      }
      if (parent.isFile()) {
        showErrorDialog("errorCreateNewDirTitle",
                        "errorCreateNewDirIsFileMessage",
                        parent);
        return false;
      }
      if (!parent.isWritable()) {
        showErrorDialog("errorCreateNewDirTitle",
                        "errorCreateNewDirPermissionMessage",
                        parent);
        return false;
      }
    }

    try {
      file.create(nsIFile.DIRECTORY_TYPE, 0755); 
    } catch (e) {
      showErrorDialog("errorCreateNewDirTitle",
                      "errorCreateNewDirMessage",
                      file);
      return false;
    }
    file.normalize(); 
    gotoDirectory(file);
    
    
    
    gNewDirName = { value: "" }; 
  }
  return true;
}

function gotoDirectory(directory) {
  window.setCursor("wait");
  try {
    populateAncestorList(directory);
    treeView.setDirectory(directory);
    document.getElementById("errorShower").selectedIndex = 0;
  } catch(ex) {
    document.getElementById("errorShower").selectedIndex = 1;
  }

  window.setCursor("auto");

  if (filePickerMode == nsIFilePicker.modeGetFolder) {
    textInput.value = "";
  }
  textInput.focus();
  textInput.setAttribute("autocompletesearchparam", directory.path);
  sfile = directory;
}

function toggleShowHidden(event) {
  treeView.showHiddenFiles = !treeView.showHiddenFiles;
}








function processPath(path)
{
  var fileArray = new Array();
  var strLength = path.length;

  if (path[0] == '"' && filePickerMode == nsIFilePicker.modeOpenMultiple &&
      strLength > 1) {
    
    

    var curFileStart = 1;

    while (1) {
      var nextQuote;

      
      var quoteSearchStart = curFileStart + 1;
      do {
        nextQuote = path.indexOf('"', quoteSearchStart);
        quoteSearchStart = nextQuote + 1;
      } while (nextQuote != -1 && path[nextQuote - 1] == '\\');
      
      if (nextQuote == -1) {
        
        

        if (!processPathEntry(path.substring(curFileStart), fileArray))
          return false;
        break;
      }

      if (!processPathEntry(path.substring(curFileStart, nextQuote), fileArray))
        return false;

      curFileStart = path.indexOf('"', nextQuote + 1);
      if (curFileStart == -1) {
        
        

        if (nextQuote < strLength - 1)
          if (!processPathEntry(path.substring(nextQuote + 1), fileArray))
            return false;
        break;
      }
      ++curFileStart;
    }
  } else {
    
    if (!processPathEntry(path, fileArray))
      return false;
  }

  return fileArray;
}

function processPathEntry(path, fileArray)
{
  var filePath;
  var file;

  try {
    file = sfile.clone().QueryInterface(nsILocalFile);
  } catch(e) {
    dump("Couldn't clone\n"+e);
    return false;
  }

  var tilde_file = file.clone();
  tilde_file.append("~");
  if (path[0] == '~' &&                        
      !(path == "~" && tilde_file.exists()) && 
      (path.length == 1 || path[1] == "/"))    
    filePath = homeDir.path + path.substring(1);
  else
    filePath = path;

  
  filePath = filePath.replace(/\\\"/g, "\"");
  
  if (filePath[0] == '/')   
    file.initWithPath(filePath);
  else if ((filePath.indexOf("/../") > 0) ||
           (filePath.substr(-3) == "/..") ||
           (filePath.substr(0,3) == "../") ||
           (filePath == "..")) {
    
    try{
      file.initWithPath(file.path + "/" + filePath);
    } catch (e) {
      dump("Couldn't init path\n"+e);
      return false;
    }
  }
  else {
    try {
      file.appendRelativePath(filePath);
    } catch (e) {
      dump("Couldn't append path\n"+e);
      return false;
    }
  }

  fileArray[fileArray.length] = file;
  return true;
}
