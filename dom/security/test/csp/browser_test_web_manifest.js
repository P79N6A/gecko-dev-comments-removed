







'use strict';
requestLongerTimeout(10); 
const {
  ManifestObtainer
} = Components.utils.import('resource://gre/modules/WebManifest.jsm', {});
const path = '/tests/dom/security/test/csp/';
const testFile = `file=${path}file_web_manifest.html`;
const remoteFile = `file=${path}file_web_manifest_remote.html`;
const httpsManifest = `file=${path}file_web_manifest_https.html`;
const mixedContent = `file=${path}file_web_manifest_mixed_content.html`;
const server = 'file_testserver.sjs';
const defaultURL = `http://example.org${path}${server}`;
const remoteURL = `http://mochi.test:8888`;
const secureURL = `https://example.com${path}${server}`;
const tests = [
  
  
  {
    expected: `default-src 'none' blocks fetching manifest.`,
    get tabURL() {
      let queryParts = [
        `csp=default-src 'none'`,
        testFile
      ];
      return `${defaultURL}?${queryParts.join('&')}`;
    },
    run(topic) {
      is(topic, 'csp-on-violate-policy', this.expected);
    }
  },
  
  
  
  {
    expected: `default-src mochi.test:8888 blocks manifest fetching.`,
    get tabURL() {
      let queryParts = [
        `csp=default-src mochi.test:8888`,
        testFile
      ];
      return `${defaultURL}?${queryParts.join('&')}`;
    },
    run(topic) {
      is(topic, 'csp-on-violate-policy', this.expected);
    }
  },
  
  
  {
    expected: `CSP default-src 'self' allows fetch of manifest.`,
    get tabURL() {
      let queryParts = [
        `csp=default-src 'self'`,
        testFile
      ];
      return `${defaultURL}?${queryParts.join('&')}`;
    },
    run(manifest) {
      is(manifest.name, 'loaded', this.expected);
    }
  },
  
  
  {
    expected: 'CSP default-src mochi.test:8888 allows fetching manifest.',
    get tabURL() {
      let queryParts = [
        `csp=default-src http://mochi.test:8888`,
        remoteFile
      ];
      return `${defaultURL}?${queryParts.join('&')}`;
    },
    run(manifest) {
      is(manifest.name, 'loaded', this.expected);
    }
  },
  
  
  
  {
    expected: `default-src 'none' blocks mochi.test:8888`,
    get tabURL() {
      let queryParts = [
        `csp=default-src 'none'`,
        remoteFile
      ];
      return `${defaultURL}?${queryParts.join('&')}`;
    },
    run(topic) {
      is(topic, 'csp-on-violate-policy', this.expected);
    }
  },
  
  {
    expected: `CSP manifest-src allows self`,
    get tabURL() {
      let queryParts = [
        `manifest-src 'self'`,
        testFile
      ];
      return `${defaultURL}?${queryParts.join('&')}`;
    },
    run(manifest) {
      is(manifest.name, 'loaded', this.expected);
    }
  },
  
  {
    expected: `CSP manifest-src allows http://example.org`,
    get tabURL() {
      let queryParts = [
        `manifest-src http://example.org`,
        testFile
      ];
      return `${defaultURL}?${queryParts.join('&')}`;
    },
    run(manifest) {
      is(manifest.name, 'loaded', this.expected);
    }
  },
  
  
  {
    expected: `CSP manifest-src overrides default-src of elsewhere.com`,
    get tabURL() {
      let queryParts = [
        `default-src: http://elsewhere.com; manifest-src http://example.org`,
        testFile
      ];
      return `${defaultURL}?${queryParts.join('&')}`;
    },
    run(manifest) {
      is(manifest.name, 'loaded', this.expected);
    }
  },
  
  
  {
    expected: `CSP manifest-src overrides default-src`,
    get tabURL() {
      let queryParts = [
        `default-src: 'none'; manifest-src 'self'`,
        testFile
      ];
      return `${defaultURL}?${queryParts.join('&')}`;
    },
    run(manifest) {
      is(manifest.name, 'loaded', this.expected);
    }
  },
  
  
  {
    expected: `CSP manifest-src allows mochi.test:8888`,
    get tabURL() {
      let queryParts = [
        `csp=default-src *; manifest-src http://mochi.test:8888`,
        remoteFile
      ];
      return `${defaultURL}?${queryParts.join('&')}`;
    },
    run(manifest) {
      is(manifest.name, 'loaded', this.expected);
    }
  },
  
  
  
  {
    expected: `CSP blocks manifest fetching from example.org.`,
    get tabURL() {
      let queryParts = [
        `csp=manifest-src mochi.test:8888`,
        testFile
      ];
      return `${defaultURL}?${queryParts.join('&')}`;
    },
    run(topic) {
      is(topic, 'csp-on-violate-policy', this.expected);
    }
  },
  
  
  
  {
    expected: `CSP manifest-src 'self' blocks cross-origin fetch.`,
    get tabURL() {
      let queryParts = [
        `csp=manifest-src 'self'`,
        remoteFile
      ];
      return `${defaultURL}?${queryParts.join('&')}`;
    },
    run(topic) {
      is(topic, 'csp-on-violate-policy', this.expected);
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
    const observer = (/blocks/.test(aTest.expected)) ? new NetworkObserver(aTest) : null;
    const obtainer = new ManifestObtainer();
    let manifest;
    
    
    try {
      manifest = yield obtainer.obtainManifest(aBrowser);
    } catch (e) {
      const msg = `Expected promise rejection obtaining.`;
      ok(/blocked the loading of a resource/.test(e.message), msg);
      if (observer) {
        yield observer.finished;
      }
      return;
    }
    
    if (manifest) {
      aTest.run(manifest);
    }
  }
});



function NetworkObserver(test) {
  let finishedTest;
  let success = false;
  this.finished = new Promise((resolver) => {
    finishedTest = resolver;
  })
  this.observe = function observer(subject, topic) {
    SpecialPowers.removeObserver(this, 'csp-on-violate-policy');
    test.run(topic);
    finishedTest();
    success = true;
  };
  SpecialPowers.addObserver(this, 'csp-on-violate-policy', false);
  setTimeout(() => {
    if (!success) {
      test.run('This test timed out.');
      finishedTest();
    }
  }, 1000);
}
