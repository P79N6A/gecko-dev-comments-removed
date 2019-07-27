



"use strict";


const FEEDBACK_PRODUCT_STRING = "Firefox for Android";

let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Messaging.jsm");
document.addEventListener("DOMContentLoaded", init, false);

function dump(a) {
  Services.console.logStringMessage(a);
}

function init() {
  let anchors = document.querySelectorAll(".maybe-later");
  for(let anchor of anchors) {
    anchor.addEventListener("click", maybeLater, false);
  }
  document.getElementById("happy-link").addEventListener("click", function(evt) {
    switchSection("happy");
  }, false);
  document.getElementById("sad-link").addEventListener("click", function(evt) {
    switchSection("sad");
  }, false);

  window.addEventListener("unload", uninit, false);

  document.getElementById("open-play-store").addEventListener("click", openPlayStore, false);
  document.forms[0].addEventListener("submit", sendFeedback, false);
  for (let anchor of document.querySelectorAll(".no-thanks")) {
    anchor.addEventListener("click", evt => window.close(), false);
  }

  let sumoLink = Services.urlFormatter.formatURLPref("app.support.baseURL");
  document.getElementById("sumo-link").href = sumoLink;

  window.addEventListener("popstate", function (aEvent) {
	updateActiveSection(aEvent.state ? aEvent.state.section : "intro")
  }, false);

  
  Services.obs.addObserver(function observer(aSubject, aTopic, aData) {
	document.getElementById("last-url").value = aData;
  }, "Feedback:LastUrl", false);

  Messaging.sendRequest({ type: "Feedback:LastUrl" });
}

function uninit() {
  Services.obs.removeObserver(this, "Feedback:LastUrl");
}

function switchSection(aSection) {
  history.pushState({ section: aSection }, aSection);
  updateActiveSection(aSection);
}

function updateActiveSection(aSection) {
  document.querySelector("section[active]").removeAttribute("active");
  document.getElementById(aSection).setAttribute("active", true);
}

function openPlayStore() {
  Messaging.sendRequest({ type: "Feedback:OpenPlayStore" });

  window.close();
}

function maybeLater() {
  window.close();

  Messaging.sendRequest({ type: "Feedback:MaybeLater" });
}

function sendFeedback(aEvent) {
  
  aEvent.preventDefault();

  let section = history.state.section;

  
  if (section != "sad") {
	Cu.reportError("Trying to send feedback from an invalid section: " + section);
	return;
  }

  let sectionElement = document.getElementById(section);
  let descriptionElement = sectionElement.querySelector(".description");

  
  
  if (!descriptionElement.validity.valid)
	return;

  let data = {};
  data["happy"] = false;
  data["description"] = descriptionElement.value;
  data["product"] = FEEDBACK_PRODUCT_STRING;

  let urlElement = document.getElementById("last-url");
  
  
  if (!urlElement.validity.valid)
	return;

  
  if (urlElement.value) {
    data["url"] = urlElement.value;
  }

  let sysInfo = Cc["@mozilla.org/system-info;1"].getService(Ci.nsIPropertyBag2);
  data["device"] = sysInfo.get("device");
  data["manufacturer"] = sysInfo.get("manufacturer");
  data["source"] = "about:feedback";

  let req = new XMLHttpRequest();
  req.addEventListener("error", function() {
	Cu.reportError("Error sending feedback to input.mozilla.org: " + req.statusText);
  }, false);
  req.addEventListener("abort", function() {
	Cu.reportError("Aborted sending feedback to input.mozilla.org: " + req.statusText);
  }, false);

  let postURL = Services.urlFormatter.formatURLPref("app.feedback.postURL");
  req.open("POST", postURL, true);
  req.setRequestHeader("Content-type", "application/json");
  req.send(JSON.stringify(data));

  switchSection("thanks-" + section);
}
