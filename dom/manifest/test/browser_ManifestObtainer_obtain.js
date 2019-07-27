

'use strict';
Cu.import('resource://gre/modules/ManifestObtainer.jsm', this);
requestLongerTimeout(4); 
const defaultURL =
  'http://example.org/tests/dom/manifest/test/resource.sjs';
const remoteURL =
  'http://mochi.test:8888/tests/dom/manifest/test/resource.sjs';
const tests = [
  
  {
    expected: 'Manifest is first `link` where @rel contains token manifest.',
    get tabURL() {
      let query = [
        `body=<h1>${this.expected}</h1>`,
        'Content-Type=text/html; charset=utf-8',
      ];
      const URL = `${defaultURL}?${query.join('&')}`;
      return URL;
    },
    run(manifest) {
      Assert.strictEqual(manifest.name, 'pass-1', this.expected);
    },
    testData: `
      <link rel="manifesto" href='${defaultURL}?body={"name":"fail"}'>
      <link rel="foo bar manifest bar test" href='${defaultURL}?body={"name":"pass-1"}'>
      <link rel="manifest" href='${defaultURL}?body={"name":"fail"}'>`
  }, {
    expected: 'Manifest is first `link` where @rel contains token manifest.',
    get tabURL() {
      let query = [
        `body=<h1>${this.expected}</h1>`,
        'Content-Type=text/html; charset=utf-8',
      ];
      const URL = `${defaultURL}?${query.join('&')}`;
      return URL;
    },
    run(manifest) {
      Assert.strictEqual(manifest.name, 'pass-2', this.expected);
    },
    testData: `
      <link rel="foo bar manifest bar test" href='resource.sjs?body={"name":"pass-2"}'>
      <link rel="manifest" href='resource.sjs?body={"name":"fail"}'>
      <link rel="manifest foo bar test" href='resource.sjs?body={"name":"fail"}'>`
  }, {
    expected: 'By default, manifest load cross-origin.',
    get tabURL() {
      let query = [
        `body=<h1>${this.expected}</h1>`,
        'Content-Type=text/html; charset=utf-8',
      ];
      const URL = `${defaultURL}?${query.join('&')}`;
      return URL;
    },
    run(manifest) {
      
      todo_is(manifest.name, 'pass-3', this.expected);
    },
    testData: `<link rel="manifest" href='${remoteURL}?body={"name":"pass-3"}'>`
  },
  
  {
    expected: 'CORS enabled, manifest must be fetched.',
    get tabURL() {
      let query = [
        `body=<h1>${this.expected}</h1>`,
        'Content-Type=text/html; charset=utf-8',
      ];
      const URL = `${defaultURL}?${query.join('&')}`;
      return URL;
    },
    run(manifest) {
      Assert.strictEqual(manifest.name, 'pass-4', this.expected);
    },
    get testData() {
      const body = 'body={"name": "pass-4"}';
      const CORS =
        `Access-Control-Allow-Origin=${new URL(this.tabURL).origin}`;
      const link =
        `<link
        crossorigin=anonymous
        rel="manifest"
        href='${remoteURL}?${body}&${CORS}'>`;
      return link;
    }
  }, {
    expected: 'Fetch blocked by CORS - origin does not match.',
    get tabURL() {
      let query = [
        `body=<h1>${this.expected}</h1>`,
        'Content-Type=text/html; charset=utf-8',
      ];
      const URL = `${defaultURL}?${query.join('&')}`;
      return URL;
    },
    run(err) {
      Assert.strictEqual(err.name, 'TypeError', this.expected);
    },
    get testData() {
      const body = 'body={"name": "fail"}';
      const CORS = 'Access-Control-Allow-Origin=http://not-here';
      const link =
        `<link
        crossorigin
        rel="manifest"
        href='${remoteURL}?${body}&${CORS}'>`;
      return link;
    }
  },
];

add_task(function*() {
  yield new Promise(resolve => {
    SpecialPowers.pushPrefEnv({
      'set': [
        ['dom.fetch.enabled', true]
      ]
    }, resolve);
  });
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
    aBrowser.contentWindowAsCPOW.document.head.innerHTML = aTest.testData;
    try {
      const manifest = yield obtainer.obtainManifest(aBrowser);
      aTest.run(manifest);
    } catch (e) {
      aTest.run(e);
    }
  }
});






add_task(function*() {
  const obtainer = new ManifestObtainer();
  const defaultPath = '/tests/dom/manifest/test/manifestLoader.html';
  const tabURLs = [
    `http://test:80${defaultPath}`,
    `http://mochi.test:8888${defaultPath}`,
    `http://test1.mochi.test:8888${defaultPath}`,
    `http://sub1.test1.mochi.test:8888${defaultPath}`,
    `http://sub2.xn--lt-uia.mochi.test:8888${defaultPath}`,
    `http://test2.mochi.test:8888${defaultPath}`,
    `http://example.org:80${defaultPath}`,
    `http://test1.example.org:80${defaultPath}`,
    `http://test2.example.org:80${defaultPath}`,
    `http://sub1.test1.example.org:80${defaultPath}`,
    `http://sub1.test2.example.org:80${defaultPath}`,
    `http://sub2.test1.example.org:80${defaultPath}`,
    `http://sub2.test2.example.org:80${defaultPath}`,
    `http://example.org:8000${defaultPath}`,
    `http://test1.example.org:8000${defaultPath}`,
    `http://test2.example.org:8000${defaultPath}`,
    `http://sub1.test1.example.org:8000${defaultPath}`,
    `http://sub1.test2.example.org:8000${defaultPath}`,
    `http://sub2.test1.example.org:8000${defaultPath}`,
    `http://sub2.test2.example.org:8000${defaultPath}`,
    `http://example.com:80${defaultPath}`,
    `http://www.example.com:80${defaultPath}`,
    `http://test1.example.com:80${defaultPath}`,
    `http://test2.example.com:80${defaultPath}`,
    `http://sub1.test1.example.com:80${defaultPath}`,
    `http://sub1.test2.example.com:80${defaultPath}`,
    `http://sub2.test1.example.com:80${defaultPath}`,
    `http://sub2.test2.example.com:80${defaultPath}`,
  ];
  
  let browsers = [
    for (url of tabURLs) gBrowser.addTab(url).linkedBrowser
  ];
  
  yield Promise.all((
    for (browser of browsers) BrowserTestUtils.browserLoaded(browser)
  ));
  
  
  const results = yield Promise.all((
    for (browser of randBrowsers(browsers, 1000)) obtainer.obtainManifest(browser)
  ));
  const expected = 'Expect every manifest to have name equal to `pass`.';
  const pass = results.every(manifest => manifest.name === 'pass');
  Assert.ok(pass, expected);
  
  browsers
    .map(browser => gBrowser.getTabForBrowser(browser))
    .forEach(tab => gBrowser.removeTab(tab));

  
  function* randBrowsers(aBrowsers, aMax) {
    for (let i = 0; i < aMax; i++) {
      const randNum = Math.round(Math.random() * (aBrowsers.length - 1));
      yield aBrowsers[randNum];
    }
  }
});
