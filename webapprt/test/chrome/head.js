



const INSTALL_URL =
  "http://mochi.test:8888/webapprtChrome/webapprt/test/chrome/install.html";

Cu.import("resource://gre/modules/Services.jsm");















function installWebapp(manifestPath, parameters, onInstall) {
  
  
  
  
  
  
  
  
  

  let content = document.getElementById("content");

  Services.obs.addObserver(function observe(subj, topic, data) {
    
    Services.obs.removeObserver(observe, "webapprt-test-did-install");
    let appConfig = JSON.parse(data);

    content.addEventListener("load", function onLoad(event) {
      
      content.removeEventListener("load", onLoad, true);
      let webappURL = appConfig.app.origin + appConfig.app.manifest.launch_path;
      is(event.target.URL, webappURL,
         "No other page should have loaded between installation and " +
         "the webapp's page load: " + event.target.URL);
      onInstall(appConfig);
    }, true);
  }, "webapprt-test-did-install", false);

  
  let args = [["manifestPath", manifestPath]];
  if (parameters !== undefined) {
    args.push(["parameters", parameters]);
  }
  let queryStr = args.map(function ([key, val])
                          key + "=" + encodeURIComponent(JSON.stringify(val))).
                 join("&");
  let installURL = INSTALL_URL + "?" + queryStr;
  content.loadURI(installURL);
}
