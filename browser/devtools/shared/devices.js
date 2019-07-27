



"use strict";

const { Ci, Cc } = require("chrome");
const { getJSON } = require("devtools/shared/getjson");
const { Services } = require("resource://gre/modules/Services.jsm");
const promise = require("promise");

const DEVICES_URL = "devtools.devices.url";
const Strings = Services.strings.createBundle("chrome://browser/locale/devtools/device.properties");
























let localDevices = {};


function AddDevice(device, type = "phones") {
  let list = localDevices[type];
  if (!list) {
    list = localDevices[type] = [];
  }
  list.push(device);
}
exports.AddDevice = AddDevice;


function GetDevices(bypassCache = false) {
  let deferred = promise.defer();

  
  getJSON(DEVICES_URL, bypassCache).then(devices => {
    for (let type in localDevices) {
      if (!devices[type]) {
        devices.TYPES.push(type);
        devices[type] = [];
      }
      devices[type] = localDevices[type].concat(devices[type]);
    }
    deferred.resolve(devices);
  });

  return deferred.promise;
}
exports.GetDevices = GetDevices;


function GetDeviceString(deviceType) {
  return Strings.GetStringFromName("device." + deviceType);
}
exports.GetDeviceString = GetDeviceString;
