





































const IO_SERVICE_CTRID = 
	"@mozilla.org/network/io-service;1" ;
const nsIFileProtocolHandler = 
 	Components.interfaces.nsIFileProtocolHandler ;
const nsIIOService = 
	Components.interfaces.nsIIOService ;

function FileProtocolHandler()
{
    try {
	    netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
	    netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

	    ioservice = Components.classes[IO_SERVICE_CTRID].getService(nsIIOService);

	    
	    if(!ioservice)
	       this.exception = "Unable to get nsIIOService" ;
	          
	    this.protocolhandler = ioservice.getProtocolHandler("file").QueryInterface(nsIFileProtocolHandler) ;

	    if (!this.protocolhandler)
	       this.exception = "Unable to get nsIFileProtocolHandler" ;
	      
	   return this ;
     }
     catch(e){
	this.exception = e ;
     }
     
	 
   
}

FileProtocolHandler.prototype.returnvalue = null;
FileProtocolHandler.prototype.success = null;
FileProtocolHandler.prototype.exception = null;
FileProtocolHandler.prototype.protocolhandler = null ;


FileProtocolHandler.prototype.newFileURI = function (aFile)
{
  







  
  try {
	 netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
	 netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

	 this.returnvalue = this.protocolhandler.newFileURI(aFile)
	  if (this.returnvalue)
	    this.success =  true ;
	  else
	    this.success =  false ;
  }
  catch(e){
    this.success =  false ;
    this.exception =  e ;
  }

}


FileProtocolHandler.prototype.getURLSpecFromFile = function (aFile)
{
 









  try {
	 
	 netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
	 netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
	
	 this.returnvalue = this.protocolhandler.getURLSpecFromFile(aFile)
	  if (this.returnvalue)
	    this.success =  true ;
	  else
	    this.success =  false ;
  }
  catch(e){
    this.success =  false ;
    this.exception =  e ;
  }
 
 }

FileProtocolHandler.prototype.getFileFromURLSpec = function (aUrl)
{
 





  
  try {
	  netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
	  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

	  this.returnvalue = this.protocolhandler.getFileFromURLSpec(aUrl)
	  if (this.returnvalue)
	    this.success =  true ;
	  else
	    this.success =  false ;
  }
  catch(e){
    this.success =  false ;
    this.exception =  e ;
  }
 
}

