





































const IO_SERVICE_CTRID = 
	"@mozilla.org/network/io-service;1" ;
const nsIIOService = 
	Components.interfaces.nsIIOService ;

function URI(aSpec,aOriginCharset,aBaseURI)
{
    try {
	    netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
	    netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
	    

	    ioservice = Components.classes[IO_SERVICE_CTRID].getService(nsIIOService);

	    
	    if(!ioservice)
	       this.exception = "Unable to get nsIIOService" ;
	          
	    this.uri = ioservice.newURI(aSpec,aOriginCharset,aBaseURI) ;

	    if (!this.uri)
	       this.exception = "Unable to get nsIURI" ;
	      
	   return this ;
     }
     catch(e){
	this.exception = e ;
     }
     
	 
   
}

URI.prototype.returnvalue = null;
URI.prototype.success = null;
URI.prototype.exception = null;
URI.prototype.uri = null ;



URI.prototype.scheme getter = function ()
{

 



  
  try {
	 netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
	 netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

	 this.returnvalue = this.uri.scheme ;
	 
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

URI.prototype.scheme setter = function (aScheme)
{

 



  
  try {
	 netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
	 netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

	 this.uri.scheme  = aScheme ;
 
	  this.returnvalue =  this.uri.scheme
	  this.success =  true ;
  }
  catch(e){
    this.success =  false ;
    this.exception =  e ;
  }

}

URI.prototype.spec getter = function ()
{

 





   
   try {
	 netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
	 netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

	 this.returnvalue = this.uri.spec ;
	 
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

URI.prototype.spec setter = function (aSpec)
{

 





   
   try {
	 netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
	 netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

	 this.uri.spec =  aSpec ; ;
	 this.returnvalue = this.uri.spec ;
	 
	 this.success =  true ;
  }
  catch(e){
    this.success =  false ;
    this.exception =  e ;
  }

}

URI.prototype.prePath getter = function ()
{

  





   
   try {
	 netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
	 netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

	 this.returnvalue = this.uri.prePath ;
	 
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


URI.prototype.userPass getter= function ()
{

 




    try {
	 netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
	 netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

	 this.returnvalue = this.uri.userPass ;
     this.success =  true ;
  }
  catch(e){
    this.success =  false ;
    this.exception =  e ;
  }
 
}

URI.prototype.userPass setter= function (aUserPass)
{

 




    try {
	 netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
	 netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

	 this.uri.userPass = aUserPass ;
	 this.returnvalue = this.uri.userPass ;
     this.success =  true ;
  }
  catch(e){
    this.success =  false ;
    this.exception =  e ;
  }
 
}

URI.prototype.username getter = function ()
{

 






 
  
   try {
	 netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
	 netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

	 this.returnvalue = this.uri.username ;
	 
	  if (this.returnvalue)
      this.success =  true ;
 }
  catch(e){
    this.success =  false ;
    this.exception =  e ;
  }

}

URI.prototype.username setter = function (aUserName)
{

 






 
  
   try {
	 netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
	 netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

	 this.uri.username = aUserName ;
	 this.returnvalue = this.uri.username ;
	 
	  if (this.returnvalue)
      this.success =  true ;
 }
  catch(e){
    this.success =  false ;
    this.exception =  e ;
  }

}

URI.prototype.password getter = function ()
{
 






 
  
  try {
 	 netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
 	 netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
 
 	 this.returnvalue = this.uri.password ;
 	 
 	  if (this.returnvalue)
      this.success =  true ;
  }
   catch(e){
     this.success =  false ;
     this.exception =  e ;
   }

}

URI.prototype.password setter = function (aPassword)
{
 






 
  
  try {
 	 netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
 	 netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
 
 	 this.uri.password = aPassword ;
 	 this.returnvalue = this.uri.password ;
 	 
 	  if (this.returnvalue)
      this.success =  true ;
  }
   catch(e){
     this.success =  false ;
     this.exception =  e ;
   }

}

URI.prototype.hostPort getter= function ()
{
  






  
  try {
 	 netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
 	 netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
 
 	 this.returnvalue = this.uri.hostPort ;
 	 
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

URI.prototype.hostPort setter= function (aHostPort)
{
  






  
  try {
 	 netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
 	 netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
 
 	 this.uri.hostPort = aHostPort ;
 	 this.returnvalue = this.uri.hostPort ;
 	 
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

URI.prototype.host getter = function ()
{
 








    try {
 	 netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
 	 netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
 
 	 this.returnvalue = this.uri.host ;
 	 
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

URI.prototype.host setter = function (aHost)
{
 








    try {
 	 netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
 	 netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
 
 	 this.uri.host = aHost;
 	 this.returnvalue = this.uri.host ;
 	 
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


URI.prototype.port getter = function ()
{
 





  
  try {
 	 netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
 	 netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
 
 	 this.returnvalue = this.uri.port ;
 	 
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

URI.prototype.port setter = function (aPort)
{
 





  
  try {
 	 netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
 	 netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
 
 	 this.uri.port = aPort ;
 	 this.returnvalue = this.uri.port ;
 	 
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


URI.prototype.path getter = function ()
{
 







  
  try {
 	 netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
 	 netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
 
 	 this.returnvalue = this.uri.path ;
 	 
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

URI.prototype.path setter = function (aPath)
{
 







  
  try {
 	 netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
 	 netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
 
 	 this.uri.path  = aPath ;
 	 this.returnvalue = this.uri.path ;
 	 
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



URI.prototype.equals = function (aURI)
{
 





  
  try {
 	 netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
 	 netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
 
 	 this.returnvalue = this.uri.equals(aURI);
 	 this.success =  true ;
   }
   catch(e){
     this.success =  false ;
     this.exception =  e ;
   }

}



URI.prototype.schemeIs = function (aScheme)
{
 






  
  try {
 	 netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
 	 netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
 
 	 this.returnvalue = this.uri.schemeIs(aScheme)	 ;
 	 this.success =  true ;
 	 
   }
   catch(e){
     this.success =  false ;
     this.exception =  e ;
   }

}



URI.prototype.clone = function ()

{
 







  
    try {
 	 netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
 	 netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
 
 	 this.returnvalue = this.uri.clone() ;
 	 
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



URI.prototype.resolve = function (aRelativePath)
{
 







  
    try {
 	 netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
 	 netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
 
 	 this.returnvalue = this.uri.resolve(aRelativePath) ;
 	 
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



URI.prototype.asciiSpec = function ()
{

 






  
 try {
	 netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
	 netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

	 this.returnvalue = this.uri.asciiSpec ;
	 
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
 
 
 
 
URI.prototype.asciiHost = function ()
{
 






  
  try {
 	 netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
 	 netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
 
 	 this.returnvalue = this.uri.asciiHost ;
 	 
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




URI.prototype.originCharset = function ()
{
 










    try {
 	 netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
 	 netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
 
 	 this.returnvalue = this.uri.originCharset ;
 	 
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
 