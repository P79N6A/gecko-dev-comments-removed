








































function WriteResults(buffer)
{

    var obj = getAppObj();
    if (!obj)
    {
       alert("Cannot use this function, You are not runnig this test case in automation framework");
       return ;
    }
    obj.WriteResults(buffer) ;
}




function isRunningStandalone()
{
   var obj = getAppObj();
   if(obj)
     return false ;
   else
     return true ;
}





function getAppObj()
{
   var obj ;

   const WIN_MEDIATOR_CTRID =
   	  "@mozilla.org/appshell/window-mediator;1";
   const nsIWindowMediator    = Components.interfaces.nsIWindowMediator;

   netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserAccess");
   netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

    var windowManager =
           Components.classes[WIN_MEDIATOR_CTRID].getService(nsIWindowMediator);
    var parentwindow = windowManager.getMostRecentWindow("Automation:framewrk");

    if (parentwindow)
    {
	   obj =  parentwindow.document.getElementById("ContentBody");
	   return obj.appobj
    }

   if (window.opener)
   {
		obj = window.opener.document.getElementById("ContentBody");
		if (obj)
		   return obj.appobj;
   }
   obj =  document.getElementById("ContentBody");

   if (obj)
	  return obj.appobj;

    var win = window.top;
    while(win)
    {
       obj = win.document.getElementById("ContentBody");
       if (obj)
		  return obj.appobj ;

       if (win == win.top)
         return null ;

       win = win.parent ;
    }
   return null ;
}
