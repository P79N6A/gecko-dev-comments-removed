













function getNormalizedPort(targetPort) {
  return ([80, 443, ""].indexOf(targetPort) >= 0) ? "" : ":" + targetPort;
}








function guid() {
  return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function(c) {
    var r = Math.random() * 16 | 0, v = c == 'x' ? r : (r & 0x3 | 0x8);
    return v.toString(16);
  });
}









function xhrRequest(url, responseType) {
  return new Promise(function(resolve, reject) {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', url, true);
    xhr.responseType = responseType || "json";

    xhr.addEventListener("error", function() {
      reject(Error("Network Error"));
    });

    xhr.addEventListener("load", function() {
      if (xhr.status != 200)
        return reject(Error(xhr.statusText));

      resolve(xhr.response);
    });

    xhr.send();
  });
}






function setAttributes(el, attrs) {
  attrs = attrs || {}
  for (var attr in attrs)
    el.setAttribute(attr, attrs[attr]);
}











function bindEvents(element, resolveEventName, rejectEventName) {
  element.eventPromise = new Promise(function(resolve, reject) {
    element.addEventListener(resolveEventName  || "load", resolve);
    element.addEventListener(rejectEventName || "error", reject);
  });
}












function createElement(tagName, attrs, parent, doBindEvents) {
  var element = document.createElement(tagName);

  if (doBindEvents)
    bindEvents(element);

  
  
  setAttributes(element, attrs);

  if (parent)
    parent.appendChild(element);

  return element;
}

function createRequestViaElement(tagName, attrs, parent) {
  return createElement(tagName, attrs, parent, true).eventPromise;
}







function createHelperIframe(name, doBindEvents) {
  return createElement("iframe",
                       {"name": name, "id": name},
                       document.body,
                       doBindEvents);
}







function requestViaIframe(url) {
  return createRequestViaElement("iframe", {"src": url}, document.body);
}







function requestViaImage(url) {
  return createRequestViaElement("img", {"src": url}, document.body);
}






function requestViaXhr(url) {
  return xhrRequest(url);
}






function requestViaFetch(url) {
  return fetch(url);
}








function requestViaWorker(url) {
  var worker = new Worker(url);
  bindEvents(worker, "message", "error");
  worker.postMessage('');

  return worker.eventPromise;
}









function requestViaNavigable(navigableElement, url) {
  var iframe = createHelperIframe(guid(), true);
  setAttributes(navigableElement,
                {"href": url,
                 "target": iframe.name});
  navigableElement.click();

  return iframe.eventPromise;
}







function requestViaAnchor(url) {
  var a = createElement("a", {"innerHTML": "Link to resource"}, document.body);

  return requestViaNavigable(a, url);
}







function requestViaArea(url) {
  var area = createElement("area", {}, document.body);

  return requestViaNavigable(area, url);
}







function requestViaScript(url) {
  return createRequestViaElement("script", {"src": url}, document.body);
}







function requestViaForm(url) {
  var iframe = createHelperIframe(guid());
  var form = createElement("form",
                           {"action": url,
                            "method": "POST",
                            "target": iframe.name},
                           document.body);
  bindEvents(iframe);
  form.submit();

  return iframe.eventPromise;
}







function requestViaLinkStylesheet(url) {
  return createRequestViaElement("link",
                                 {"rel": "stylesheet", "href": url},
                                 document.head);
}







function requestViaLinkPrefetch(url) {
  
  
  
  return createRequestViaElement("link",
                                 {"rel": "prefetch", "href": url},
                                 document.head);
}









function createMediaElement(type, media_attrs, source_attrs) {
  var mediaElement = createElement(type, {});
  var sourceElement = createElement("source", {}, mediaElement);

  mediaElement.eventPromise = new Promise(function(resolve, reject) {
    mediaElement.addEventListener("loadeddata", resolve);
    
    sourceElement.addEventListener("error", reject);
  });

  setAttributes(mediaElement, media_attrs);
  setAttributes(sourceElement, source_attrs);
  document.body.appendChild(mediaElement);

  return mediaElement;
}







function requestViaVideo(url) {
  return createMediaElement("video",
                            {},
                            {type: "video/mp4", src: url}).eventPromise;
}







function requestViaAudio(url) {
  return createMediaElement("audio",
                            {},
                            {type: "audio/mpeg", src: url}).eventPromise;
}








function requestViaPicture(url) {
  var picture = createMediaElement("picture", {}, {"srcset": url,
                                                "type": "image/png"});
  return createRequestViaElement("img", {"src": url}, picture);
}







function requestViaObject(url) {
  return createRequestViaElement("object", {"data": url}, document.body);
}



function SanityChecker() {}
SanityChecker.prototype.checkScenario = function() {};
