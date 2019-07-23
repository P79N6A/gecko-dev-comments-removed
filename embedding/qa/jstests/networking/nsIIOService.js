





































const nsIIOService =
     Components.interfaces.nsIIOService
const IO_SERVICE_CTRID =
      "@mozilla.org/network/io-service;1" ;


function IOService()
{
   
   try{
   	netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
   	netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
   
       
       this.service  = Components.classes[IO_SERVICE_CTRID].getService(nsIIOService);
       if (!this.service)
       	   return null ;
       return this ;
   }
   catch(e)
   {
     this.exception = e ;
     return null;
   }
}


IOService.prototype.service =  null ;
IOService.prototype.success =  null ;
IOService.prototype.exception =  null ;
IOService.prototype.returnvalue =  null ;


IOService.prototype.getProtocolHandler = function(aScheme)
{

	 








	  try{

		netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
		netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

		this.returnvalue  = this.service.getProtocolHandler(aScheme) ;
		
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

IOService.prototype.getProtocolFlags= function(aScheme)
{
	 









	  try{

		netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
		netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

		this.returnvalue  = this.service.getProtocolFlags(aScheme) ;
		if (this.returnvalue>=0)
		  this.success = true
		else
		  this.success = false;
	 }
	 catch(e){   
		this.success = false;
		this.exception = e ;
	  }
}

IOService.prototype.newURI =  function(aSpec,aOriginCharset,aBaseURI)
{
	 












	  try{

		netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
		netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

		this.returnvalue  = this.service.newURI(aSpec,aOriginCharset,aBaseURI) ;
		
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


IOService.prototype.newFileURI= function(file)
{
	 








	  try{

		netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
		netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

		this.returnvalue  = this.service.newFileURI(file) ;
		
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


IOService.prototype.newChannelFromURI = function(aURI)
{
	 








	  try{

		netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
		netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

		this.returnvalue  = this.service.newChannelFromURI(aURI) ;
		
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


IOService.prototype.newChannel = function(aSpec,aOriginCharset,aBaseURI)
{
	 







	  try{

		netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
		netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

		this.returnvalue  = this.service.newChannel(aSpec,aOriginCharset,aBaseURI) ;
		
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

IOService.prototype.Getoffline = function()
{

	 









	  try{

		netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
		netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

		this.returnvalue  = this.service.offline ;
		this.success = true;
	 }
	 catch(e){   
		this.success = false;
		this.exception = e ;
	  }

}

IOService.prototype. Setoffline = function(aOffline)
{
	 








	  
	  try{

		netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
		netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
		
		this.service.offline  = aOffline ;
		this.returnvalue  = this.service.offline 
		this.success = true;
	 }
	 catch(e){   
		this.success = false;
		this.exception = e ;
	  }

}

IOService.prototype.allowPort= function(aPort, aScheme)
{
	 














	  try{

		netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
		netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

		this.returnvalue  = this.service.allowPort(aPort, aScheme) ;
		
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

IOService.prototype.extractScheme = function(urlString)
{
	 















	  try
	  {

		netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
		netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

		this.returnvalue  = this.service.extractScheme(urlString) ;
		
		if (this.returnvalue)
		  this.success = true
		else
		  this.success = false;
	 }
	 catch(e)
	 {   
		this.success = false;
		this.exception = e ;
	 }

}
