









































function GetPublishSiteData()
{
  var publishBranch = GetPublishPrefsBranch();
  if (!publishBranch)
    return null;

  
  var siteNameList = GetSiteNameList(true, false);
  if (!siteNameList)
    return null;

  
  var siteArray = [];

  
  
  try {
    publishBranch.deleteBranch("site_name.");
  } catch (e) {}

  
  var index = 0;
  for (var i = 0; i < siteNameList.length; i++)
  {
    
    var publishData = GetPublishData_internal(publishBranch, siteNameList[i]);
    if (publishData)
    {
      siteArray[index] = publishData;
      SetPublishStringPref(publishBranch, "site_name."+index, siteNameList[i]);
      index++;
    }
    else
    {
      try {
        
        publishBranch.deleteBranch("site_data." + siteNameList[i] + ".");
      } catch (e) {}
    }
  }

  SavePrefFile();

  if (index == 0) 
    return null;


  return siteArray;
}

function GetDefaultPublishSiteName()
{
  var publishBranch = GetPublishPrefsBranch();
  var name = "";  
  if (publishBranch)
    name = GetPublishStringPref(publishBranch, "default_site");

  return name;
}



function CreatePublishDataFromUrl(docUrl)
{
  if (!docUrl || IsUrlAboutBlank(docUrl) || GetScheme(docUrl) == "file")
    return null;

  var pubSiteData = GetPublishSiteData();
  if (pubSiteData)
  {
    var dirObj = {};
    var index = FindSiteIndexAndDocDir(pubSiteData, docUrl, dirObj);
    var publishData;
    if (index != -1)
    {
      publishData = pubSiteData[index];
      publishData.docDir = FormatDirForPublishing(dirObj.value)

      
      
      publishData.otherDir = FormatDirForPublishing(pubSiteData[index].otherDir);

      publishData.filename = GetFilename(docUrl);
      publishData.notInSiteData = false;
      return publishData;
    }
  }

  
  

  
  var userObj = {};
  var passObj = {};
  var pubUrl = StripUsernamePassword(docUrl, userObj, passObj);

  
  var lastSlash = pubUrl.lastIndexOf("\/");
  
  pubUrl = pubUrl.slice(0, lastSlash+1);

  var siteName = CreateSiteNameFromUrl(pubUrl, pubSiteData);

  publishData = { 
    siteName : siteName,
    previousSiteName : siteName,
    filename : GetFilename(docUrl),
    username : userObj.value,
    password : passObj.value,
    savePassword : false,
    publishUrl : pubUrl,
    browseUrl  : pubUrl,
    docDir     : "",
    otherDir   : "",
    publishOtherFiles : true,
    dirList    : [""],
    saveDirs   : false,
    notInSiteData : true
  }

  return publishData;
}

function CreateSiteNameFromUrl(url, publishSiteData)
{
  var host = GetHost(url);
  var schemePostfix = " (" + GetScheme(url) + ")";
  var siteName = host + schemePostfix;

  if (publishSiteData)
  {
    
    var i = 1;
    var exists = false;
    do {
      exists = PublishSiteNameExists(siteName, publishSiteData, -1)  
      if (exists)
        siteName = host + "-" + i + schemePostfix;
      i++;
    }
    while (exists);
  }
  return siteName;
}






function GetPublishDataFromSiteName(siteName, docUrlOrFilename)
{
  var publishBranch = GetPublishPrefsBranch();
  if (!publishBranch)
    return null;

  var siteNameList = GetSiteNameList(false, false);
  if (!siteNameList)
    return null;
  for (var i = 0; i < siteNameList.length; i++)
  {
    if (siteNameList[i] == siteName)
    {
      var publishData = GetPublishData_internal(publishBranch, siteName);
      if (GetScheme(docUrlOrFilename))
        FillInMatchingPublishData(publishData, docUrlOrFilename);
      else
        publishData.filename = docUrlOrFilename;

      return publishData;
    }
  }
  return null;
}

function GetDefaultPublishData()
{
  var publishBranch = GetPublishPrefsBranch();
  if (!publishBranch)
    return null;

  var siteName = GetPublishStringPref(publishBranch, "default_site");
  if (!siteName)
    return null;

  return GetPublishData_internal(publishBranch, siteName);
}

function GetPublishData_internal(publishBranch, siteName)
{
  if (!publishBranch || !siteName)
    return null;

  var prefPrefix = "site_data." + siteName + ".";

  
  
  
  var publishUrl = GetPublishStringPref(publishBranch, prefPrefix+"url");
  if (!publishUrl)
    return null;

  var savePassword = false;
  var publishOtherFiles = true;
  try {
    savePassword = publishBranch.getBoolPref(prefPrefix+"save_password");
    publishOtherFiles = publishBranch.getBoolPref(prefPrefix+"publish_other_files");
  } catch (e) {}

  var publishData = { 
    siteName : siteName,
    previousSiteName : siteName,
    filename : "",
    username : GetPublishStringPref(publishBranch, prefPrefix+"username"),
    savePassword : savePassword,
    publishUrl : publishUrl,
    browseUrl  : GetPublishStringPref(publishBranch, prefPrefix+"browse_url"),
    docDir     : FormatDirForPublishing(GetPublishStringPref(publishBranch, prefPrefix+"doc_dir")),
    otherDir   : FormatDirForPublishing(GetPublishStringPref(publishBranch, prefPrefix+"other_dir")),
    publishOtherFiles : publishOtherFiles,
    saveDirs : false
  }

  
  publishData.password = GetSavedPassword(publishData);

  
  
  if (publishData.password)
  {
    if (!savePassword)
    {
      try {
        publishPrefsBranch.setBoolPref(prefPrefix+"save_password", true);
      } catch (e) {}
    }
    publishData.savePassword = true;
  }

  
  
  publishData.dirList = [""];

  
  var dirCount = {value:0};
  var dirPrefs;
  try {
    dirPrefs = publishBranch.getChildList(prefPrefix+"dir.", dirCount);
  } catch (e) {}

  if (dirPrefs && dirCount.value > 0)
  {
    if (dirCount.value > 1)
      dirPrefs.sort();

    for (var j = 0; j < dirCount.value; j++)
    {
      var dirName = GetPublishStringPref(publishBranch, dirPrefs[j]);
      if (dirName)
        publishData.dirList[j+1] = dirName;
    }
  }

  return publishData;
}




function SavePublishSiteDataToPrefs(siteArray, defaultName)
{
  var publishBranch = GetPublishPrefsBranch();
  if (!publishBranch)
    return false;

  try {
    if (siteArray)
    {
      var defaultFound = false;

      
      publishBranch.deleteBranch("site_name.");
      publishBranch.deleteBranch("site_data.");

      for (var i = 0; i < siteArray.length; i++)
      {
        SavePublishData_Internal(publishBranch, siteArray[i], i);
        if (!defaultFound)
          defaultFound = defaultName == siteArray[i].siteName;
      }
      
      if (siteArray.length && !defaultFound)
        defaultName = siteArray[0].siteName;
    }

    
    SetPublishStringPref(publishBranch, "default_site", defaultName);
  
    
    SavePrefFile();
  }
  catch (ex) { return false; }

  return true;
}



function SavePublishDataToPrefs(publishData)
{
  if (!publishData || !publishData.publishUrl)
    return false;

  var publishBranch = GetPublishPrefsBranch();
  if (!publishBranch)
    return false;

  
  if (!publishData.siteName)
    publishData.siteName = CreateSiteNameFromUrl(publishData.publishUrl, publishData);

  var siteCount = {value:0};
  var siteNamePrefs;
  try {
    siteNamePrefs = publishBranch.getChildList("site_name.", siteCount);
  } catch (e) {}

  if (!siteNamePrefs || siteCount.value == 0)
  {
    
    var siteData = [publishData];
    return SavePublishSiteDataToPrefs(siteData, publishData.siteName);
  }

  
  var previousSiteName =  ("previousSiteName" in publishData && publishData.previousSiteName) ? 
                            publishData.previousSiteName : publishData.siteName;

  
  
  for (var i = 0; i < siteCount.value; i++)
  {
    var siteName = GetPublishStringPref(publishBranch, "site_name."+i);

    if (siteName == previousSiteName)
    {
      
      try {
        publishBranch.deleteBranch("site_data." + siteName + ".");
      } catch (e) {}
      break;
    }
  }

  
  publishData.previousSiteName = publishData.siteName;

  var ret = SavePublishData_Internal(publishBranch, publishData, i);
  if (ret)
  {
    
    var defaultSiteName = GetPublishStringPref(publishBranch, "default_site");
    if (previousSiteName == defaultSiteName 
        && publishData.siteName != defaultSiteName)
      SetPublishStringPref(publishBranch, "default_site", publishData.siteName);

    SavePrefFile();

    
    if ("notInSiteData" in publishData && publishData.notInSiteData)
      publishData.notInSiteData = false;
  }
  return ret;
}


function SavePublishData_Internal(publishPrefsBranch, publishData, siteIndex)
{
  if (!publishPrefsBranch || !publishData)
    return false;

  SetPublishStringPref(publishPrefsBranch, "site_name."+siteIndex, publishData.siteName);

  FixupUsernamePasswordInPublishData(publishData);

  var prefPrefix = "site_data." + publishData.siteName + "."

  SetPublishStringPref(publishPrefsBranch, prefPrefix+"url", publishData.publishUrl);
  SetPublishStringPref(publishPrefsBranch, prefPrefix+"browse_url", publishData.browseUrl);
  SetPublishStringPref(publishPrefsBranch, prefPrefix+"username", publishData.username);
  
  try {
    publishPrefsBranch.setBoolPref(prefPrefix+"save_password", publishData.savePassword);
    publishPrefsBranch.setBoolPref(prefPrefix+"publish_other_files", publishData.publishOtherFiles);
  } catch (e) {}

  
  
  SavePassword(publishData);

  SetPublishStringPref(publishPrefsBranch, prefPrefix+"doc_dir", 
                       FormatDirForPublishing(publishData.docDir));

  if (publishData.publishOtherFiles && publishData.otherDir)
    SetPublishStringPref(publishPrefsBranch, prefPrefix+"other_dir",
                         FormatDirForPublishing(publishData.otherDir));

  if ("saveDirs" in publishData && publishData.saveDirs)
  {
    if (publishData.docDir)
      AppendNewDirToList(publishData, publishData.docDir);

    if (publishData.publishOtherFiles && publishData.otherDir 
        && publishData.otherDir != publishData.docDir)
      AppendNewDirToList(publishData, publishData.otherDir);
  }

  
  if (publishData.dirList.length)
  {
    publishData.dirList.sort();
    var dirIndex = 0;
    for (var j = 0; j < publishData.dirList.length; j++)
    {
      var dir = publishData.dirList[j];

      
      if (dir && dir != "/")
      {
        SetPublishStringPref(publishPrefsBranch, prefPrefix + "dir." + dirIndex, dir);
        dirIndex++;
      }
    }
  }

  return true;
}

function AppendNewDirToList(publishData, newDir)
{
  newDir = FormatDirForPublishing(newDir);
  if (!publishData || !newDir)
    return;

  if (!publishData.dirList)
  {
    publishData.dirList = [newDir];
    return;
  }

  
  for (var i = 0; i < publishData.dirList.length; i++)
  {
    
    if (newDir == publishData.dirList[i])
      return;
  }
  
  publishData.dirList[publishData.dirList.length] = newDir;
}

function RemovePublishSubdirectoryFromPrefs(publishData, removeDir)
{
  removeDir = FormatDirForPublishing(removeDir);
  if (!publishData || !publishData.siteName || !removeDir)
    return false;

  var publishBranch = GetPublishPrefsBranch();
  if (!publishBranch)
    return false;

  var prefPrefix = "site_data." + publishData.siteName + ".";

  
  if (publishData.docDir == removeDir)
  {
    publishData.docDir = "";
    SetPublishStringPref(publishBranch, prefPrefix+"doc_dir", "");
  }

  if (publishData.otherDir == removeDir)
  {
    publishData.otherDir = "";
    SetPublishStringPref(publishBranch, prefPrefix+"other_dir", "");
  }

  prefPrefix += "dir.";

  
  try {
    publishBranch.deleteBranch(prefPrefix);
  } catch (e) {}

  
  if (publishData.dirList.length)
  {
    var dirIndex = 0;
    var docDirInList = false;
    var otherDirInList = false;
    for (var i = 0; i < publishData.dirList.length; i++)
    {
      var dir = publishData.dirList[i];
      if (dir == removeDir)
      {
        
        publishData.dirList.splice(i, 1);
        --i;
      }
      else if (dir && dir != "/") 
      {
        
        SetPublishStringPref(publishBranch, prefPrefix + dirIndex, dir);
        dirIndex++;
      }
    }
  }
  SavePrefFile();
  return true;
}

function SetDefaultSiteName(name)
{
  if (name)
  {
    var publishBranch = GetPublishPrefsBranch();
    if (publishBranch)
      SetPublishStringPref(publishBranch, "default_site", name);

    SavePrefFile();
  }
}

function SavePrefFile()
{
  try {
    if (gPrefsService)
      gPrefsService.savePrefFile(null);
  }
  catch (e) {}
}



function GetPublishPrefsBranch()
{
  var prefsService = GetPrefsService();
  if (!prefsService)
    return null;

  return prefsService.getBranch("editor.publish.");
}

function GetSiteNameList(doSort, defaultFirst)
{
  var publishBranch = GetPublishPrefsBranch();
  if (!publishBranch)
    return null;

  var siteCountObj = {value:0};
  var siteNamePrefs;
  try {
    siteNamePrefs = publishBranch.getChildList("site_name.", siteCountObj);
  } catch (e) {}

  if (!siteNamePrefs || siteCountObj.value == 0)
    return null;

  
  var siteNameList = [];
  var index = 0;
  var defaultName = "";
  if (defaultFirst)
  {
    defaultName = GetPublishStringPref(publishBranch, "default_site");
    
    siteNameList[0] = "";
    index++;
  }

  for (var i = 0; i < siteCountObj.value; i++)
  {
    var siteName = GetPublishStringPref(publishBranch, siteNamePrefs[i]);
    
    if (siteName && siteName != defaultName)
    {
      siteNameList[index] = siteName;
      index++;
    }
  }

  if (siteNameList.length && doSort)
    siteNameList.sort();

  if (defaultName)
  {
    siteNameList[0] = defaultName;
    index++;
  }

  return siteNameList.length? siteNameList : null;
}

function PublishSiteNameExists(name, publishSiteData, skipSiteIndex)
{
  if (!name)
    return false;

  if (!publishSiteData)
  {
    publishSiteData = GetPublishSiteData();
    skipSiteIndex = -1;
  }

  if (!publishSiteData)
    return false;

  
  for (var i = 0; i < publishSiteData.length; i++)
  {
    if (i != skipSiteIndex && name == publishSiteData[i].siteName)
      return true;
  }
  return false;
}









function FindSiteIndexAndDocDir(publishSiteData, docUrl, dirObj)
{
  if (dirObj)
    dirObj.value = "";

  if (!publishSiteData || !docUrl || GetScheme(docUrl) == "file")
    return -1;

  var siteIndex = -1;
  var siteUrlLen = 0;
  
  for (var i = 0; i < publishSiteData.length; i++)
  {
    
    
    
    
    var lenObj = {value:0};
    var tempData = Clone(publishSiteData[i]);

    
    var len = FillInMatchingPublishData(tempData, docUrl);

    if (len > siteUrlLen)
    {
      siteIndex = i;
      siteUrlLen = len;
      if (dirObj)
        dirObj.value = tempData.docDir;

      
    }
  }
  return siteIndex;
}







function FillInMatchingPublishData(publishData, docUrl)
{
  if (!publishData || !docUrl)
    return 0;

  
  var lastSlash = docUrl.lastIndexOf("\/");
  var baseUrl = docUrl.slice(0, lastSlash+1);
  var filename = docUrl.slice(lastSlash+1);
    
  
  
  
  
  var username = {value:""};
  baseUrl = StripUsernamePassword(baseUrl, username); 
  username = username.value;

  var matchedLength = 0;
  var pubUrlFound = publishData.publishUrl ?
                      baseUrl.indexOf(publishData.publishUrl) == 0 : false;
  var browseUrlFound = publishData.browseUrl ?
                          baseUrl.indexOf(publishData.browseUrl) == 0 : false;

  if ((pubUrlFound || browseUrlFound) 
      && (!username || !publishData.username || username == publishData.username))
  {
    
    matchedLength = pubUrlFound ? publishData.publishUrl.length 
                            : publishData.browseUrl.length;

    if (matchedLength > 0)
    {
      publishData.filename = filename;

      
      publishData.docDir = FormatDirForPublishing(baseUrl.slice(matchedLength));
    }
  }
  return matchedLength;
}



function GetPublishStringPref(prefBranch, name)
{
  if (prefBranch && name)
  {
    try {
      return prefBranch.getComplexValue(name, Components.interfaces.nsISupportsString).data;
    } catch (e) {}
  }
  return "";
}

function SetPublishStringPref(prefBranch, name, value)
{
  if (prefBranch && name)
  {
    try {
        var str = Components.classes["@mozilla.org/supports-string;1"]
                            .createInstance(Components.interfaces.nsISupportsString);
        str.data = value;
        prefBranch.setComplexValue(name, Components.interfaces.nsISupportsString, str);
    } catch (e) {}
  }
}




function FormatUrlForPublishing(url)
{
  url = TrimString(StripUsernamePassword(url));
  if (url)
  {
    var lastChar = url.charAt(url.length-1);
    if (lastChar != "/" && lastChar != "=" && lastChar != "&" && lastChar  != "?")
      return (url + "/");
  }
  return url;
}





function FixupUsernamePasswordInPublishData(publishData)
{
  var ret = false;
  if (publishData && publishData.publishUrl)
  {
    var userObj = {value:""};
    var passObj = {value:""};
    publishData.publishUrl = FormatUrlForPublishing(StripUsernamePassword(publishData.publishUrl, userObj, passObj));
    if (userObj.value)
    {
      publishData.username = userObj.value;
      ret = true;
    }
    if (passObj.value)
    {
      publishData.password = passObj.value;
      ret = true;
    }
    
    publishData.browseUrl = FormatUrlForPublishing(publishData.browseUrl);
  }
  return ret;
}



function FormatDirForPublishing(dir)
{
  dir = TrimString(dir);

  
  
  if (!dir || dir == "/" || dir == "//")
    return "";

  
  if (dir.charAt(0) == "/")
    dir = dir.slice(1);

  
  var dirLen = dir.length;
  var lastChar = dir.charAt(dirLen-1);
  if (dirLen > 1 && lastChar != "/" && lastChar != "=" && lastChar != "&" && lastChar  != "?")
    return (dir + "/");

  return dir;
}


var gPasswordManager;
function GetPasswordManager()
{
  if (!gPasswordManager)
  {
    var passwordManager = Components.classes["@mozilla.org/passwordmanager;1"].createInstance();
    if (passwordManager)
      gPasswordManager = passwordManager.QueryInterface(Components.interfaces.nsIPasswordManager);
  }
  return gPasswordManager;
}

var gPasswordManagerInternal;
function GetPasswordManagerInternal()
{
  if (!gPasswordManagerInternal)
  {
    try {
      gPasswordManagerInternal =
        Components.classes["@mozilla.org/passwordmanager;1"].createInstance(
          Components.interfaces.nsIPasswordManagerInternal);
    } catch (e) {
    }
  }
  return gPasswordManagerInternal;
}

function GetSavedPassword(publishData)
{
  if (!publishData)
    return "";
  var passwordManagerInternal = GetPasswordManagerInternal();
  if (!passwordManagerInternal)
    return "";

  var host = {value:""};
  var user =  {value:""};
  var password = {value:""}; 
  var url = GetUrlForPasswordManager(publishData);
  
  try {
    passwordManagerInternal.findPasswordEntry
      (url, publishData.username, "", host, user, password);
    return password.value;
  } catch (e) {}

  return "";
}

function SavePassword(publishData)
{
  if (!publishData || !publishData.publishUrl || !publishData.username)
    return false;

  var passwordManager = GetPasswordManager();
  if (passwordManager)
  {
    var url = GetUrlForPasswordManager(publishData);

    
    
    try {
      passwordManager.removeUser(url, publishData.username);
    } catch (e) {}

    
    if (publishData.savePassword)
    {
      try {
        passwordManager.addUser(url, publishData.username, publishData.password);
      } catch (e) {}
    }
    return true;
  }
  return false;
}

function GetUrlForPasswordManager(publishData)
{
  if (!publishData || !publishData.publishUrl)
    return false;

  var url;

  
  
  if (publishData.username && GetScheme(publishData.publishUrl) == "ftp")
    url = InsertUsernameIntoUrl(publishData.publishUrl, publishData.username);
  else
    url = publishData.publishUrl;

  
  var len = url.length;
  if (len && url.charAt(len-1) == "\/")
    url = url.slice(0, len-1);
  
  return url;
}
