














































function sHistoryInit(theWindow)
{
  var sHistory = null;
  var ifaceReq;
  var webNav;
  
  try
  {       
    netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserAccess");
    netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");       
  
    try {
       ifaceReq = theWindow.QueryInterface(Components.interfaces.nsIInterfaceRequestor);
    }
    catch(e){
       alert("Unable to get interface requestor object: " + e);
       return null;
    }
    try {
       webNav = ifaceReq.getInterface(Components.interfaces.nsIWebNavigation);
    }
    catch(e) {
      alert("Unable to get WebNavigation: " + e);
      return null;
    }               
    try {
       sHistory = webNav.sessionHistory;
    }
    catch(e) {
      alert("Didn't get SessionHistory object: " + e);
      return null;
    } 
    return sHistory;
  }
  catch(e) 
  {
    alert("Could not find Session History component: " + e);
    return null;
  }
}




function testHistoryCount(sHistory)
{
  if (!sHistory) {
    alert("Didn't get SessionHistory object");
    return 0;
  }
  var cnt = sHistory.count;     
  return cnt; 
}






function testHistoryIndex(sHistory)
{
  if (!sHistory) {
    alert("Didn't get SessionHistory object");
    return -1;
  }
  var shIndex = sHistory.index;   
  return shIndex;
}




function testMaxLength(sHistory)
{
  if (!sHistory) {
    alert("Didn't get SessionHistory object");
    return 0;
  }
  var maxLen = sHistory.maxLength;  
  return maxLen;
}







function testGetEntryAtIndex(sHistory, cnt, modIndex)
{
  if (!sHistory) {
    alert("Didn't get SessionHistory object");
    return null;
  }
  var entry = sHistory.getEntryAtIndex(cnt, modIndex);

  return entry;
}


function  testSimpleEnum(sHistory)


{
  if (!sHistory) {
    alert("Didn't get SessionHistory object");
    return null;
  }
   var simpleEnum = sHistory.SHistoryEnumerator;
   
   return simpleEnum;
}






function  testHistoryEntry(entry, index)
{
  if (!entry)
    alert("Didn't get history entry object");

   
   Uri = entry.URI;
   uriSpec = Uri.spec;
   dump("The URI = " + uriSpec);

   
   title = entry.title;
   dump("The page title = " + title);
    
   
   frameStatus = entry.isSubFrame;
   dump("The subframe value = " + frameStatus);
}





function  testHistoryEntryUri(nextHE, index)
{
  if (!nextHE) {
    alert("Didn't get history entry object");
    return null;
  }
   
   Uri = entry.URI;
   uriSpec = Uri.spec;
   
   return uriSpec;
}





function  testHistoryEntryTitle(nextHE, index)
{
  if (!nextHE) {
    alert("Didn't get history entry object");
    return null;
  }
   
   title = entry.title;
   
   return title;
}





function  testHistoryEntryFrame(nextHE, index)
{
  if (!nextHE) {
    alert("Didn't get history entry object");
    return null;
  }
  
   frameStatus = entry.isSubFrame;
   
   return frameStatus;
}
  
 



function purgeEntries(sHistory, numEntries)
{
  if (!sHistory) {
    alert("Didn't get SessionHistory object"); 
  }         
  sHistory.PurgeHistory(numEntries);        
} 





function setSHistoryListener(sHistory, listener)
{
  dump("Inside nsISHistoryTestLib.js: setSHistoryListener()");
  if (!sHistory) {
    alert("Didn't get SessionHistory object");
  }
    netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserAccess");
    netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");       
    sHistory.addSHistoryListener(listener);
}