






































function openCertManager()
{
    
    const kWindowMediatorContractID = "@mozilla.org/appshell/window-mediator;1";
    const kWindowMediatorIID = Components.interfaces.nsIWindowMediator;
    const kWindowMediator = Components.classes[kWindowMediatorContractID].getService(kWindowMediatorIID);
    var lastCertManager = kWindowMediator.getMostRecentWindow("mozilla:certmanager");
    if (lastCertManager)
      lastCertManager.focus();
    else {
      window.open('chrome://pippki/content/certManager.xul',  "",
                  'chrome,centerscreen,resizable,dialog');
    }
}

function openDeviceManager()
{
    
    const kWindowMediatorContractID = "@mozilla.org/appshell/window-mediator;1";
    const kWindowMediatorIID = Components.interfaces.nsIWindowMediator;
    const kWindowMediator = Components.classes[kWindowMediatorContractID].getService(kWindowMediatorIID);
    var lastCertManager = kWindowMediator.getMostRecentWindow("mozilla:devicemanager");
    if (lastCertManager)
      lastCertManager.focus();
    else {
      window.open('chrome://pippki/content/device_manager.xul',  "devmgr",
                  'chrome,centerscreen,resizable,dialog');
    }
}
