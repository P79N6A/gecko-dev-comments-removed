












































  
try
{
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

  if (!this.Cc)
    this.Cc = Components.classes;
  if (!this.Ci)
    this.Ci = Components.interfaces;
  if (!this.Cr)
    this.Cr = Components.results;
  
  const PROXY_AUTOCONFIG_PREF = "network.proxy.autoconfig_url";
  const PROXY_TYPE_PREF = "network.proxy.type";
  const PROXY_TYPE_USE_PAC = 2;
  
  
  



  function DomainMapper()
  {
    



    this._mappings = {};
  
    
    this._oldProxyType = undefined;
    
    
    this._oldPAC = undefined;
  
    
    this._enabled = false;  
  }
  DomainMapper.prototype =
  {
    



    addMapping: function(fromHost, fromPort, toHost, toPort)
    {
      this._mappings[fromHost + ":" + fromPort] = toHost + ":" + toPort;
    },
    
    



    removeMapping: function(host, port)
    {
      delete this._mappings[host + ":" + port];
    },
    
    
    get isEnabled()
    {
      return this._enabled;
    },
  
    


    enable: function()
    {
      if (this._enabled)
      {
        this.syncMappings();
        return;
      }
      
      var prefs = Cc["@mozilla.org/preferences-service;1"]
                    .getService(Ci.nsIPrefBranch);
        
      
      try
      {
        this._oldPAC = prefs.getCharPref(PROXY_AUTOCONFIG_PREF);
      }
      catch (e)
      {
        this._oldPAC = undefined;
      }
    
      
      try
      {
        this._oldProxyType = prefs.getIntPref(PROXY_TYPE_PREF);
        prefs.setIntPref(PROXY_TYPE_PREF, PROXY_TYPE_USE_PAC);
      }
      catch (e)
      {
        this._oldProxyType = 0;
      }
      
      this._enabled = true;
      
      this.syncMappings();
    },
    
    






    syncMappings: function()
    {
      if (!this._enabled)
        throw Cr.NS_ERROR_UNEXPECTED;
  
      var url = "data:text/plain,";
      url += "function FindProxyForURL(url, host)                  ";
      url += "{                                                    ";
      url += "  var mappings = " + this._mappings.toSource() + ";  ";
      url += "  var regex = new RegExp('http://(.*?(:\\\\d+)?)/'); ";
      url += "  var matches = regex.exec(url);                     ";
      url += "  var hostport = matches[1], port = matches[2];      ";
      url += "  if (!port)                                         ";
      url += "    hostport += ':80';                               ";
      url += "  if (hostport in mappings)                          ";
      url += "    return 'PROXY ' + mappings[hostport];            ";
      url += "  return 'DIRECT';                                   ";
      url += "}";
      
      Cc["@mozilla.org/preferences-service;1"]
        .getService(Ci.nsIPrefBranch)
        .setCharPref(PROXY_AUTOCONFIG_PREF, url);
    },
  
    
    disable: function()
    {
      if (!this._enabled)
        return;
  
      var prefs = Cc["@mozilla.org/preferences-service;1"]
                    .getService(Ci.nsIPrefBranch);
        
      if (this._oldPAC !== undefined)
        prefs.setCharPref(PROXY_AUTOCONFIG_PREF, this._oldPAC);
      this._oldPAC = undefined;
  
      prefs.setIntPref(PROXY_TYPE_PREF, this._oldProxyType);
      this._oldProxyType = undefined;
  
      this._enabled = false;
    }
  };
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  var mappedHostPorts =
    [
     {host: "example.org", port: 80},
     {host: "test1.example.org", port: 80},
     {host: "test2.example.org", port: 80},
     {host: "sub1.test1.example.org", port: 80},
     {host: "sub1.test2.example.org", port: 80},
     {host: "sub2.test1.example.org", port: 80},
     {host: "sub2.test2.example.org", port: 80},
     {host: "example.org", port: 8000},
     {host: "test1.example.org", port: 8000},
     {host: "test2.example.org", port: 8000},
     {host: "sub1.test1.example.org", port: 8000},
     {host: "sub1.test2.example.org", port: 8000},
     {host: "sub2.test1.example.org", port: 8000},
     {host: "sub2.test2.example.org", port: 8000},
     {host: "example.com", port: 80},
     {host: "test1.example.com", port: 80},
     {host: "test2.example.com", port: 80},
     {host: "sub1.test1.example.com", port: 80},
     {host: "sub1.test2.example.com", port: 80},
     {host: "sub2.test1.example.com", port: 80},
     {host: "sub2.test2.example.com", port: 80},
    ];

  var crossDomain = new DomainMapper();
  for (var i = 0, sz = mappedHostPorts.length; i < sz; i++)
  {
    var hostPort = mappedHostPorts[i];
    crossDomain.addMapping(hostPort.host, hostPort.port, "localhost", 8888);
  }
  crossDomain.enable();
}
catch (e)
{
  throw "privilege failure enabling cross-domain: " + e;
}
