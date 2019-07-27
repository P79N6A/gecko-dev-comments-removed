



"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/Log.jsm");

this.EXPORTED_SYMBOLS = ["CardDavImporter"];

let log = Log.repository.getLogger("Loop.Importer.CardDAV");
log.level = Log.Level.Debug;
log.addAppender(new Log.ConsoleAppender(new Log.BasicFormatter()));

const DEPTH_RESOURCE_ONLY = "0";
const DEPTH_RESOURCE_AND_CHILDREN = "1";
const DEPTH_RESOURCE_AND_ALL_DESCENDENTS = "infinity";

this.CardDavImporter = function() {
};




















this.CardDavImporter.prototype = {
  
























  startImport: function(options, callback, db) {
    let auth;
    if (!("auth" in options)) {
      callback(new Error("No authentication specified"));
      return;
    }

    if (options.auth === "basic") {
      if (!("user" in options) || !("password" in options)) {
        callback(new Error("Missing user or password for basic authentication"));
        return;
      }
      auth = { method: "basic",
               user: options.user,
               password: options.password };
    } else {
      callback(new Error("Unknown authentication method"));
      return;
    }

    if (!("host" in options)){
      callback(new Error("Missing host for CardDav import"));
      return;
    }
    let host = options.host;

    Task.spawn(function* () {
      log.info("Starting CardDAV import from " + host);
      let baseURL = "https://" + host;
      let startURL = baseURL + "/.well-known/carddav";
      let abookURL;

      
      let body = "<d:propfind xmlns:d='DAV:'><d:prop><d:getetag />" +
                 "</d:prop></d:propfind>";
      let abook = yield this._davPromise("PROPFIND", startURL, auth,
                                         DEPTH_RESOURCE_AND_CHILDREN, body);

      
      let contactElements = abook.responseXML.
                            getElementsByTagNameNS("DAV:", "href");

      body = "<c:addressbook-multiget xmlns:d='DAV:' " +
             "xmlns:c='urn:ietf:params:xml:ns:carddav'>" +
             "<d:prop><d:getetag /> <c:address-data /></d:prop>\n";

      for (let element of contactElements) {
        let href = element.textContent;
        if (href.substr(-1) == "/") {
          abookURL = baseURL + href;
        } else {
          body += "<d:href>" + href + "</d:href>\n";
        }
      }
      body += "</c:addressbook-multiget>";

      
      let allEntries = yield this._davPromise("REPORT", abookURL, auth,
                                              DEPTH_RESOURCE_AND_CHILDREN,
                                              body);

      
      let addressData = allEntries.responseXML.getElementsByTagNameNS(
        "urn:ietf:params:xml:ns:carddav", "address-data");

      log.info("Retreived " + addressData.length + " contacts from " +
                   host + "; importing into database");

      let importCount = 0;
      for (let i = 0; i < addressData.length; i++) {
        let vcard = addressData.item(i).textContent;
        let contact = this._convertVcard(vcard);
        contact.id += "@" + host;
        contact.category = ["carddav@" + host];

        let existing = yield this._dbPromise(db, "getByServiceId", contact.id);
        if (existing) {
          yield this._dbPromise(db, "remove", existing._guid);
        }

        
        
        if (!("tel" in contact) && !("email" in contact)) {
          continue;
        }

        yield this._dbPromise(db, "add", contact);
        importCount++;
      }

      return importCount;
    }.bind(this)).then(
      (result) => {
        log.info("Import complete: " + result + " contacts imported.");
        callback(null, result);
      },
      (error) => {
        log.error("Aborting import: " + error.fileName + ":" +
                      error.lineNumber + ": " + error.message);
        callback(error);
    }).then(null,
      (error) => {
        log.error("Error in callback: " + error.fileName +
                      ":" + error.lineNumber + ": " + error.message);
      callback(error);
    }).then(null,
      (error) => {
        log.error("Error calling failure callback, giving up: " +
                      error.fileName + ":" + error.lineNumber + ": " +
                      error.message);
    });
  },

  











  _dbPromise: function(db, method, param) {
    return new Promise((resolve, reject) => {
      db[method](param, (error, result) => {
        if (error) {
          reject(error);
        } else {
          resolve(result);
        }
      });
    });
  },

  








  _convertVcard: function(vcard) {
    let contact = {};
    let nickname;
    vcard.split(/[\r\n]+(?! )/).forEach(
      function (contentline) {
        contentline = contentline.replace(/[\r\n]+ /g, "");
        let match = /^(.*?[^\\]):(.*)$/.exec(contentline);
        if (match) {
          let nameparam = match[1];
          let value = match[2];

          
          value = value.replace(/\\:/g, ":");
          value = value.replace(/\\,/g, ",");
          value = value.replace(/\\n/gi, "\n");
          value = value.replace(/\\\\/g, "\\");

          let param = nameparam.split(/;/);
          let name = param[0];
          let pref = false;
          let type = [];

          for (let i = 1; i < param.length; i++) {
            if (/^PREF/.exec(param[i]) || /^TYPE=PREF/.exec(param[i])) {
              pref = true;
            }
            let typeMatch = /^TYPE=(.*)/.exec(param[i]);
            if (typeMatch) {
              type.push(typeMatch[1].toLowerCase());
            }
          }

          if (!type.length) {
            type.push("other");
          }

          if (name === "FN") {
            value = value.replace(/\\;/g, ";");
            contact.name = [value];
          }

          if (name === "N") {
            
            
            
            
            value = value.replace(/\\;/g, "\r");
            value = value.replace(/;/g, "\n");
            value = value.replace(/\r/g, ";");

            let family, given, additional, prefix, suffix;
            let values = value.split(/\n/);
            if (values.length >= 5) {
              [family, given, additional, prefix, suffix] = values;
              if (prefix.length) {
                contact.honorificPrefix = [prefix];
              }
              if (given.length) {
                contact.givenName = [given];
              }
              if (additional.length) {
                contact.additionalName = [additional];
              }
              if (family.length) {
                contact.familyName = [family];
              }
              if (suffix.length) {
                contact.honorificSuffix = [suffix];
              }
            }
          }

          if (name === "EMAIL") {
            value = value.replace(/\\;/g, ";");
            if (!("email" in contact)) {
              contact.email = [];
            }
            contact.email.push({
              pref: pref,
              type: type,
              value: value
            });
          }

          if (name === "NICKNAME") {
            value = value.replace(/\\;/g, ";");
            
            
            
            nickname = value;
          };

          if (name === "ADR") {
            value = value.replace(/\\;/g, "\r");
            value = value.replace(/;/g, "\n");
            value = value.replace(/\r/g, ";");
            let pobox, extra, street, locality, region, code, country;
            let values = value.split(/\n/);
            if (values.length >= 7) {
              [pobox, extra, street, locality, region, code, country] = values;
              if (!("adr" in contact)) {
                contact.adr = [];
              }
              contact.adr.push({
                pref: pref,
                type: type,
                streetAddress: (street || pobox) + (extra ? (" " + extra) : ""),
                locality: locality,
                region: region,
                postalCode: code,
                countryName: country
              });
            }
          }

          if (name === "TEL") {
            value = value.replace(/\\;/g, ";");
            if (!("tel" in contact)) {
              contact.tel = [];
            }
            contact.tel.push({
              pref: pref,
              type: type,
              value: value
            });
          }

          if (name === "ORG") {
            value = value.replace(/\\;/g, "\r");
            value = value.replace(/;/g, "\n");
            value = value.replace(/\r/g, ";");
            if (!("org" in contact)) {
              contact.org = [];
            }
            contact.org.push(value.replace(/\n.*/, ""));
          }

          if (name === "TITLE") {
            value = value.replace(/\\;/g, ";");
            if (!("jobTitle" in contact)) {
              contact.jobTitle = [];
            }
            contact.jobTitle.push(value);
          }

          if (name === "BDAY") {
            value = value.replace(/\\;/g, ";");
            contact.bday = Date.parse(value);
          }

          if (name === "UID") {
            contact.id = value;
          }

          if (name === "NOTE") {
            value = value.replace(/\\;/g, ";");
            if (!("note" in contact)) {
              contact.note = [];
            }
            contact.note.push(value);
          }

        }
      }
    );

    
    if (!("name" in contact) || contact.name[0].length == 0) {
      if (("familyName" in contact) && ("givenName" in contact)) {
        
        
        
        
        contact.name = [contact.familyName[0] + ", " + contact.givenName[0]];
        if (("additionalName" in contact)) {
          contact.name[0] += " " + contact.additionalName[0];
        }
      } else {
        if (nickname) {
          contact.name = [nickname];
        } else if ("familyName" in contact) {
          contact.name = [contact.familyName[0]];
        } else if ("givenName" in contact) {
          contact.name = [contact.givenName[0]];
        } else if ("org" in contact) {
          contact.name = [contact.org[0]];
        } else if ("email" in contact) {
          contact.name = [contact.email[0].value];
        } else if ("tel" in contact) {
          contact.name = [contact.tel[0].value];
        }
      }
    }

    return contact;
  },

  














  _davPromise: function(method, url, auth, depth, body) {
    return new Promise((resolve, reject) => {
      let req = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].
                createInstance(Ci.nsIXMLHttpRequest);
      let user = "";
      let password = "";

      if (auth.method == "basic") {
        user = auth.user;
        password = auth.password;
      }

      req.open(method, url, true, user, password);

      req.setRequestHeader("Depth", depth);
      req.setRequestHeader("Content-Type", "application/xml; charset=utf-8");

      req.onload = function() {
        if (req.status < 400) {
          resolve(req);
        } else {
          reject(new Error(req.status + " " + req.statusText));
        }
      };

      req.onerror = function(error) {
        reject(error);
      }

      req.send(body);
    });
  }
};
