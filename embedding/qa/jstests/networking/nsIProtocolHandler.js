





































const IO_SERVICE_CTRID = 
	"@mozilla.org/network/io-service;1" ;
const nsIIOService = 
	Components.interfaces.nsIIOService ;

function FileProtocolHandler(aScheme)
{
    try {
	    netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
	    netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
	    

	    ioservice = Components.classes[IO_SERVICE_CTRID].getService(nsIIOService);

	    
	    if(!ioservice)
	       this.exception = "Unable to get nsIIOService" ;
	          
	    this.protocolhandler = ioservice.getProtocolHandler(aScheme) ;

	    if (!this.protocolhandler)
	       this.exception = "Unable to get nsIProtocolHandler" ;
	      
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



FileProtocolHandler.prototype.scheme = function ()
{

   


  
  try {
	 netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
	 netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

	 this.returnvalue = this.protocolhandler.scheme ;
	 
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

FileProtocolHandler.prototype.defaultPort = function ()
{

   




   
   try {
	 netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
	 netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

	 this.returnvalue = this.protocolhandler.defaultPort ;
	 
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

FileProtocolHandler.prototype.protocolFlags = function ()
{

   


   
   try {
	 netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
	 netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

	 this.returnvalue = this.protocolhandler.protocolFlags ;
	 
	  if (this.returnvalue>=0)
	    this.success =  true ;
	  else
	    this.success =  false ;
  }
  catch(e){
    this.success =  false ;
    this.exception =  e ;
  }

}


FileProtocolHandler.prototype.newURI =  function(aSpec,aOriginCharset,aBaseURI)
{

  
























   
   
  try {
	 netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
	 netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

	 this.returnvalue = this.protocolhandler.newURI(aSpec,aOriginCharset,aBaseURI)
	 
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

FileProtocolHandler.prototype.newChannel = function(aURI)
{
	 








	  try{

		netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
		netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

		this.returnvalue  = this.protocolhandler.newChannel(aURI) ;
		
		if (this.returnvalue)
		  this.success = true
		else
		  this.success = false;
	 }
	 catch(e){   
		this.success = false;
		this.exception = e ;
	  }

}

FileProtocolHandler.prototype.allowPort= function(aPort, aScheme)
{
	 














	  try{

		netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
		netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

		this.returnvalue  = this.protocolhandler.allowPort(aPort, aScheme) ;
		this.success = true
	 }
	 catch(e){   
		this.success = false;
		this.exception = e ;
	  }

}
