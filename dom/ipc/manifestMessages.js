













'use strict';
const {
  utils: Cu
} = Components;
const {
  ManifestProcessor
} = Cu.import('resource://gre/modules/WebManifest.jsm', {});
const {
  Task: {
    spawn, async
  }
} = Components.utils.import('resource://gre/modules/Task.jsm', {});

addMessageListener('DOM:ManifestObtainer:Obtain', async(function* (aMsg) {
  const response = {
    msgId: aMsg.data.msgId,
    success: true,
    result: undefined
  };
  try {
    response.result = yield fetchManifest();
  } catch (err) {
    response.result = cloneError(err);
  }
  sendAsyncMessage('DOM:ManifestObtainer:Obtain', response);
}));

function cloneError(aError) {
  const clone = {
    'fileName': String(aError.fileName),
    'lineNumber': String(aError.lineNumber),
    'columnNumber': String(aError.columnNumber),
    'stack': String(aError.stack),
    'message': String(aError.message),
    'name': String(aError.name)
  };
  return clone;
}

function fetchManifest() {
  return spawn(function* () {
    if (!content || content.top !== content) {
      let msg = 'Content window must be a top-level browsing context.';
      throw new Error(msg);
    }
    const elem = content.document.querySelector('link[rel~="manifest"]');
    if (!elem || !elem.getAttribute('href')) {
      let msg = 'No manifest to fetch.';
      throw new Error(msg);
    }
    
    const manifestURL = new content.URL(elem.href, elem.baseURI);
    const reqInit = {
      mode: 'cors'
    };
    if (elem.crossOrigin === 'use-credentials') {
      reqInit.credentials = 'include';
    }
    const req = new content.Request(manifestURL, reqInit);
    req.setContext('manifest');
    const response = yield content.fetch(req);
    const manifest = yield processResponse(response, content);
    return manifest;
  });
}

function processResponse(aResp, aContentWindow) {
  return spawn(function* () {
    const badStatus = aResp.status < 200 || aResp.status >= 300;
    if (aResp.type === 'error' || badStatus) {
      let msg =
        `Fetch error: ${aResp.status} - ${aResp.statusText} at ${aResp.url}`;
      throw new Error(msg);
    }
    const text = yield aResp.text();
    const args = {
      jsonText: text,
      manifestURL: aResp.url,
      docURL: aContentWindow.location.href
    };
    const processor = new ManifestProcessor();
    const manifest = processor.process(args);
    return Cu.cloneInto(manifest, content);
  });
}
