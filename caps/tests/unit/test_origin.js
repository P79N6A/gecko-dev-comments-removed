const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
var ssm = Services.scriptSecurityManager;
function makeURI(uri) { return Services.io.newURI(uri, null, null); }

function checkCrossOrigin(a, b) {
  do_check_false(a.equals(b));
  do_check_false(a.equalsConsideringDomain(b));
  do_check_false(a.subsumes(b));
  do_check_false(a.subsumesConsideringDomain(b));
  do_check_false(b.subsumes(a));
  do_check_false(b.subsumesConsideringDomain(a));
  do_check_eq(a.cookieJar === b.cookieJar,
              a.originAttributes.appId == b.originAttributes.appId &&
              a.originAttributes.inBrowser == b.originAttributes.inBrowser);
}

function checkOriginAttributes(prin, appId, inBrowser, suffix) {
  do_check_eq(prin.originAttributes.appId, appId || 0);
  do_check_eq(prin.originAttributes.inBrowser, inBrowser || false);
  do_check_eq(prin.originSuffix, suffix || '');
}

function run_test() {
  
  do_check_eq(ssm.getSystemPrincipal().origin, '[System Principal]');
  checkOriginAttributes(ssm.getSystemPrincipal());
  var exampleOrg = ssm.createCodebasePrincipal(makeURI('http://example.org'), {});
  do_check_eq(exampleOrg.origin, 'http://example.org');
  checkOriginAttributes(exampleOrg);
  var exampleCom = ssm.createCodebasePrincipal(makeURI('https://www.example.com:123'), {});
  do_check_eq(exampleCom.origin, 'https://www.example.com:123');
  checkOriginAttributes(exampleCom);
  var nullPrin = Cu.getObjectPrincipal(new Cu.Sandbox(null));
  do_check_true(/^moz-nullprincipal:\{([0-9]|[a-z]|\-){36}\}$/.test(nullPrin.origin));
  checkOriginAttributes(nullPrin);
  var ep = Cu.getObjectPrincipal(new Cu.Sandbox([exampleCom, nullPrin, exampleOrg]));
  checkOriginAttributes(ep);
  checkCrossOrigin(exampleCom, exampleOrg);
  checkCrossOrigin(exampleOrg, nullPrin);

  
  do_check_eq(ep.origin, `[Expanded Principal [${exampleOrg.origin}, ${exampleCom.origin}, ${nullPrin.origin}]]`);

  
  do_check_true(exampleOrg.equals(Cu.getObjectPrincipal(new Cu.Sandbox('http://example.org'))));

  
  
  

  
  var exampleOrg_app = ssm.createCodebasePrincipal(makeURI('http://example.org'), {appId: 42});
  var nullPrin_app = ssm.createNullPrincipal({appId: 42});
  checkOriginAttributes(exampleOrg_app, 42, false, '!appId=42');
  checkOriginAttributes(nullPrin_app, 42, false, '!appId=42');
  do_check_eq(exampleOrg_app.origin, 'http://example.org!appId=42');

  
  var exampleOrg_browser = ssm.createCodebasePrincipal(makeURI('http://example.org'), {inBrowser: true});
  var nullPrin_browser = ssm.createNullPrincipal({inBrowser: true});
  checkOriginAttributes(exampleOrg_browser, 0, true, '!inBrowser=1');
  checkOriginAttributes(nullPrin_browser, 0, true, '!inBrowser=1');
  do_check_eq(exampleOrg_browser.origin, 'http://example.org!inBrowser=1');

  
  var exampleOrg_appBrowser = ssm.createCodebasePrincipal(makeURI('http://example.org'), {inBrowser: true, appId: 42});
  var nullPrin_appBrowser = ssm.createNullPrincipal({inBrowser: true, appId: 42});
  checkOriginAttributes(exampleOrg_appBrowser, 42, true, '!appId=42&inBrowser=1');
  checkOriginAttributes(nullPrin_appBrowser, 42, true, '!appId=42&inBrowser=1');
  do_check_eq(exampleOrg_appBrowser.origin, 'http://example.org!appId=42&inBrowser=1');

  
  var exampleCom_appBrowser = ssm.createCodebasePrincipal(makeURI('https://www.example.com:123'), {appId: 42, inBrowser: true});
  checkOriginAttributes(exampleCom_appBrowser, 42, true, '!appId=42&inBrowser=1');
  do_check_eq(exampleCom_appBrowser.origin, 'https://www.example.com:123!appId=42&inBrowser=1');

  
  checkCrossOrigin(exampleOrg_app, exampleOrg);
  checkCrossOrigin(exampleOrg_app, nullPrin_app);
  checkCrossOrigin(exampleOrg_browser, exampleOrg_app);
  checkCrossOrigin(exampleOrg_browser, nullPrin_browser);
  checkCrossOrigin(exampleOrg_appBrowser, exampleOrg_app);
  checkCrossOrigin(exampleOrg_appBrowser, nullPrin_appBrowser);
  checkCrossOrigin(exampleOrg_appBrowser, exampleCom_appBrowser);
}
