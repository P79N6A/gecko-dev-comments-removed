





'use strict';
const {
  ManifestObtainer
} = Cu.import('resource://gre/modules/ManifestObtainer.jsm', this); 
const obtainer = new ManifestObtainer();
const path = '/tests/dom/security/test/csp/';
const mixedContent = `file=${path}file_web_manifest_mixed_content.html`;
const server = 'file_testserver.sjs';
const secureURL = `https://example.com${path}${server}`;
const tests = [
  
  
  {
    expected: `Mixed Content Blocker prevents fetching manifest.`,
    get tabURL() {
      let queryParts = [
        mixedContent
      ];
      return `${secureURL}?${queryParts.join('&')}`;
    },
    run(error) {
      
      const check = /blocked the loading of a resource/.test(error.message);
      ok(check, this.expected);
    }
  }
];


add_task(function* () {
  
  for (let test of tests) {
    let tabOptions = {
      gBrowser: gBrowser,
      url: test.tabURL,
    };
    yield BrowserTestUtils.withNewTab(
      tabOptions,
      browser => testObtainingManifest(browser, test)
    );
  }

  function* testObtainingManifest(aBrowser, aTest) {
    let manifest;
    try {
      yield obtainer.obtainManifest(aBrowser);
    } catch (e) {
      return aTest.run(e);
    }
  }
});
