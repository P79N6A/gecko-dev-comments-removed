













'use strict';
const {
  utils: Cu,
} = Components;
Cu.import('resource://gre/modules/ManifestObtainer.jsm');
Cu.import('resource://gre/modules/ManifestFinder.jsm');
Cu.import('resource://gre/modules/Task.jsm');

const finder = new ManifestFinder();

const MessageHandler = {
  registerListeners() {
    addMessageListener(
      'DOM:WebManifest:hasManifestLink',
      this.hasManifestLink.bind(this)
    );
    addMessageListener(
      'DOM:ManifestObtainer:Obtain',
      this.obtainManifest.bind(this)
    );
  },

  



  hasManifestLink: Task.async(function* ({data: {id}}) {
    const response = this.makeMsgResponse(id);
    response.result = yield finder.hasManifestLink(content);
    response.success = true;
    sendAsyncMessage('DOM:WebManifest:hasManifestLink', response);
  }),

  




  obtainManifest: Task.async(function* ({data: {id}}) {
    const obtainer = new ManifestObtainer();
    const response = this.makeMsgResponse(id);
    try {
      response.result = yield obtainer.obtainManifest(content);
      response.success = true;
    } catch (err) {
      response.result = this.serializeError(err);
    }
    sendAsyncMessage('DOM:ManifestObtainer:Obtain', response);
  }),

  makeMsgResponse(aId) {
    return {
      id: aId,
      success: false,
      result: undefined
    };
  },

  






  serializeError(aError) {
    const clone = {
      'fileName': aError.fileName,
      'lineNumber': aError.lineNumber,
      'columnNumber': aError.columnNumber,
      'stack': aError.stack,
      'message': aError.message,
      'name': aError.name
    };
    return clone;
  },
};
MessageHandler.registerListeners();
