















Components.utils.import('resource://gre/modules/Services.jsm');
Components.utils.import('chrome://shumway/content/SpecialInflate.jsm');
Components.utils.import('chrome://shumway/content/RtmpUtils.jsm');

var externalInterfaceWrapper = {
  callback: function (call) {
    if (!shumwayComAdapter.onExternalCallback) {
      return undefined;
    }
    return shumwayComAdapter.onExternalCallback(
      Components.utils.cloneInto(JSON.parse(call), content));
  }
};



var shumwayComAdapter;

function sendMessage(action, data, sync, callbackCookie) {
  var detail = {action: action, data: data, sync: sync};
  if (callbackCookie !== undefined) {
    detail.callback = true;
    detail.cookie = callbackCookie;
  }
  if (!sync) {
    sendAsyncMessage('Shumway:message', detail);
    return;
  }
  var result = sendSyncMessage('Shumway:message', detail);
  return Components.utils.cloneInto(result, content);
}

function enableDebug() {
  sendAsyncMessage('Shumway:enableDebug', null);
}

addMessageListener('Shumway:init', function (message) {
  sendAsyncMessage('Shumway:running', {}, {
    externalInterface: externalInterfaceWrapper
  });

  
  
  shumwayComAdapter = Components.utils.createObjectIn(content, {defineAs: 'ShumwayCom'});
  Components.utils.exportFunction(sendMessage, shumwayComAdapter, {defineAs: 'sendMessage'});
  Components.utils.exportFunction(enableDebug, shumwayComAdapter, {defineAs: 'enableDebug'});
  Object.defineProperties(shumwayComAdapter, {
    onLoadFileCallback: { value: null, writable: true },
    onExternalCallback: { value: null, writable: true },
    onMessageCallback: { value: null, writable: true }
  });
  Components.utils.makeObjectPropsNormal(shumwayComAdapter);

  
  
  if (SpecialInflateUtils.isSpecialInflateEnabled) {
    Components.utils.exportFunction(function () {
      return SpecialInflateUtils.createWrappedSpecialInflate(content);
    }, content, {defineAs: 'createSpecialInflate'});
  }

  if (RtmpUtils.isRtmpEnabled) {
    Components.utils.exportFunction(function (params) {
      return RtmpUtils.createSocket(content, params);
    }, content, {defineAs: 'createRtmpSocket'});
    Components.utils.exportFunction(function () {
      return RtmpUtils.createXHR(content);
    }, content, {defineAs: 'createRtmpXHR'});
  }

  content.wrappedJSObject.runViewer();
});

addMessageListener('Shumway:loadFile', function (message) {
  if (!shumwayComAdapter.onLoadFileCallback) {
    return;
  }
  shumwayComAdapter.onLoadFileCallback(Components.utils.cloneInto(message.data, content));
});

addMessageListener('Shumway:messageCallback', function (message) {
  if (!shumwayComAdapter.onMessageCallback) {
    return;
  }
  shumwayComAdapter.onMessageCallback(message.data.cookie,
    Components.utils.cloneInto(message.data.response, content));
});
