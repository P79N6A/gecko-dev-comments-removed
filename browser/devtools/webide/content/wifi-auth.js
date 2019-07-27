



"use strict";

const Cu = Components.utils;
const { Services } = Cu.import("resource://gre/modules/Services.jsm");
const { require } =
  Cu.import("resource://gre/modules/devtools/Loader.jsm", {}).devtools;
const QR = require("devtools/toolkit/qrcode/index");
const PROPERTIES = "chrome://browser/locale/devtools/webide.properties";
const Strings = Services.strings.createBundle(PROPERTIES);

window.addEventListener("load", function onLoad() {
  window.removeEventListener("load", onLoad);
  document.getElementById("close").onclick = () => window.close();
  document.getElementById("no-scanner").onclick = showToken;
  document.getElementById("yes-scanner").onclick = hideToken;
  buildUI();
});

function buildUI() {
  let { oob } = window.arguments[0];
  createQR(oob);
  createToken(oob);
}

function createQR(oob) {
  let oobData = JSON.stringify(oob);
  let imgData = QR.encodeToDataURI(oobData, "L" );
  document.querySelector("#qr-code img").src = imgData.src;
}

function createToken(oob) {
  let spacedCert = oob.sha256.replace(/:/g, " ").toLowerCase();
  let certText = Strings.formatStringFromName("wifi_auth_cert",
                                              [spacedCert], 1);
  document.querySelector("#cert").textContent = certText;
  let token = oob.sha256.replace(/:/g, "").toLowerCase() + oob.k;
  document.querySelector("#token pre").textContent = token;
}

function showToken() {
  document.querySelector("body").setAttribute("token", "true");
}

function hideToken() {
  document.querySelector("body").removeAttribute("token");
}
