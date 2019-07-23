











































function webNavInit(theWindow)
{
  
  var webNav = null;
  try
  {       
    netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserAccess");
    netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");       
  
    var ifaceReq = theWindow.QueryInterface(Components.interfaces.nsIInterfaceRequestor);    
    if (!ifaceReq) {
       alert("Unable to get interface requestor object");
       return false;
    }
    webNav = ifaceReq.getInterface(Components.interfaces.nsIWebNavigation);
 
    if(!webNav) {
      alert("Unable to get WebNavigation");
      return false;
    }               
 
    return webNav;
  }
  catch(e) 
  {
    alert("Could not find Web Navigation component" + e);
    return false;
  }
}





function testCanGoBack(webNav, navEvent)
{
  if (!webNav) {
    alert("Didn't get web navigation object");
    return false;  
  }
  
  netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserAccess");
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
  
  var back = webNav.canGoBack;  	
  if (back == true && navEvent == 1)
  {
    dump("goBack value : " + back  + "\n");  
    setTimeout("testGoBack(webNav, 0)");
  }
  
  return back; 
}





function testCanGoForward(webNav, navEvent)
{
  if (!webNav) {
    alert("Didn't get web navigation object");
    return false;  
  }
  
  netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserAccess");
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
  
  var forward = webNav.canGoForward;   
  if (forward == true && navEvent == 1)
  {
    dump("goForward value : " + forward  + "\n");
    setTimeout("testGoForward(webNav, 0)", 10000);
  }
  
  return forward;
}




function testGoBack(webNav)
{
  dump("inside testGoBack()\n");

  if (!webNav) {
    alert("Didn't get web navigation object");
    return false;  
  }
  netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserAccess");
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
    
  webNav.goBack();  		
}




function testGoForward(webNav)
{
  dump("inside testGoForward()\n"); 
  netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserAccess");
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
   
  if (!webNav) {
    alert("Didn't get web navigation object");
    return false;  
  }
   
  webNav.goForward();  	
}





function testGotoIndex(webNav, index)
{
  dump("inside testGotoIndex()\n");  
  if (!webNav) {
    alert("Didn't get web navigation object");
    return false;  
  }
  netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserAccess");
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
  
  webNav.gotoIndex(index);
}






function testLoadURI(webNav, uri, loadFlags, referrer, postData, headers)
{
 
  if (!webNav) {
    alert("Didn't get web navigation object");
    return false;  
  }
  netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserAccess");
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
  
  webNav.loadURI(uri, loadFlags, referrer, postData, headers);
  getCurrURI = testCurrentURI(webNav);
  dump("the loaded uri = " + getCurrURI.spec + "\n");
}




function testReload(webNav, loadFlags)
{
    dump("inside testReload()\n");
    if (!webNav) {
      alert("Didn't get web navigation object");
      return false;  
    }
  netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserAccess");
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
    
    webNav.reload(loadFlags);
}




function testStop(webNav, stopFlags)
{
  if (!webNav) {
      alert("Didn't get web navigation object");
      return false;  
  }
  
  webNav.stop(stopFlags);
}




function testDocument(webNav)
{
  if (!webNav) {
    alert("Didn't get web navigation object");
    return false;  
  }
   
   getDoc = webNav.document;
   
   return getDoc;
}




function  testCurrentURI(webNav)
{
  if (!webNav) {
    alert("Didn't get web navigation object");
    return false;  
  }
  
   getCurrURI = webNav.currentURI;
   
   return getCurrURI;
}






















function testSessionHistory(webNav)
{
  if (!webNav) {
    alert("Didn't get web navigation object");
    return null;
  }
   
  getSHistory = webNav.sessionHistory; 
  
  return getSHistory;
}




