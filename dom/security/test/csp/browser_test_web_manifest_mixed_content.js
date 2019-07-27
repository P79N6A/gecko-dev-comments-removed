




'use strict';
const {
  ManifestObtainer
} = Components.utils.import('resource://gre/modules/WebManifest.jsm', {});
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


add_task(function*() {
  
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
    const obtainer = new ManifestObtainer();
    try {
      yield obtainer.obtainManifest(aBrowser);
    } catch (e) {
      aTest.run(e)
    }
  }
});
