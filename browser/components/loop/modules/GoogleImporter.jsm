



"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Timer.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Log",
                                  "resource://gre/modules/Log.jsm");

this.EXPORTED_SYMBOLS = ["GoogleImporter"];

let log = Log.repository.getLogger("Loop.Importer.Google");
log.level = Log.Level.Debug;
log.addAppender(new Log.ConsoleAppender(new Log.BasicFormatter()));



















const extractFieldsFromNode = function(fieldMap, node, ns = null, target = {}, wrapInArray = false) {
  for (let [field, nodeName] of fieldMap) {
    let nodeList = ns ? node.getElementsByTagNameNS(ns, nodeName) :
                        node.getElementsByTagName(nodeName);
    if (nodeList.length) {
      if (!nodeList[0].firstChild) {
        continue;
      }
      let value = nodeList[0].textContent;
      target[field] = wrapInArray ? [value] : value;
    }
  }
  return target;
};








const getFieldType = function(node) {
  if (node.hasAttribute("rel")) {
    let rel = node.getAttribute("rel");
    
    return rel.substr(rel.lastIndexOf("#") + 1);
  }
  if (node.hasAttribute("label")) {
    return node.getAttribute("label");
  }
  return "other";
};









const getPreferred = function(contact, which = "email") {
  if (!(which in contact) || !contact[which].length) {
    throw new Error("No " + which + " entry available.");
  }
  let preferred = contact[which][0];
  contact[which].some(function(entry) {
    if (entry.pref) {
      preferred = entry;
      return true;
    }
    return false;
  });
  return preferred;
};









const getUrlParam = function(paramValue, prefName, encode = true) {
  if (Services.prefs.getPrefType(prefName))
    paramValue = Services.prefs.getCharPref(prefName);
  paramValue = Services.urlFormatter.formatURL(paramValue);

  return encode ? encodeURIComponent(paramValue) : paramValue;
};

let gAuthWindow, gProfileId;
const kAuthWindowSize = {
  width: 420,
  height: 460
};
const kContactsMaxResults = 10000000;
const kContactsChunkSize = 100;
const kTitlebarPollTimeout = 200;
const kNS_GD = "http://schemas.google.com/g/2005";









this.GoogleImporter = function() {};

this.GoogleImporter.prototype = {
  





























  startImport: function(options, callback, db, windowRef) {
    Task.spawn(function* () {
      let code = yield this._promiseAuthCode(windowRef);
      let tokenSet = yield this._promiseTokenSet(code);
      let contactEntries = yield this._getContactEntries(tokenSet);
      let {total, success, ids} = yield this._processContacts(contactEntries, db, tokenSet);
      yield this._purgeContacts(ids, db);

      return {
        total: total,
        success: success
      };
    }.bind(this)).then(stats => callback(null, stats),
                       error => callback(error))
                 .then(null, ex => log.error(ex.fileName + ":" + ex.lineNumber + ": " + ex.message));
  },

  











  _promiseAuthCode: Task.async(function* (windowRef) {
    
    if (gAuthWindow && !gAuthWindow.closed) {
      gAuthWindow.close();
      gAuthWindow = null;
    }

    let url = getUrlParam("https://accounts.google.com/o/oauth2/",
                          "loop.oauth.google.URL", false) +
              "auth?response_type=code&client_id=" +
              getUrlParam("%GOOGLE_OAUTH_API_CLIENTID%", "loop.oauth.google.clientIdOverride");
    for (let param of ["redirect_uri", "scope"]) {
      url += "&" + param + "=" + encodeURIComponent(
             Services.prefs.getCharPref("loop.oauth.google." + param));
    }
    const features = "centerscreen,resizable=yes,toolbar=no,menubar=no,status=no,directories=no," +
                     "width=" + kAuthWindowSize.width + ",height=" + kAuthWindowSize.height;
    gAuthWindow = windowRef.openDialog(windowRef.getBrowserURL(), "_blank", features, url);
    gAuthWindow.focus();

    let code;

    function promiseTimeOut() {
      return new Promise(resolve => {
        setTimeout(resolve, kTitlebarPollTimeout);
      });
    }

    
    
    
    while (!code) {
      if (!gAuthWindow || gAuthWindow.closed) {
        throw new Error("Popup window was closed before authentication succeeded");
      }

      let matches = gAuthWindow.document.title.match(/(error|code)=([^\s]+)/);
      if (matches && matches.length) {
        let [, type, message] = matches;
        gAuthWindow.close();
        gAuthWindow = null;
        if (type == "error") {
          throw new Error("Google authentication failed with error: " + message.trim());
        } else if (type == "code") {
          code = message.trim();
        } else {
          throw new Error("Unknown response from Google");
        }
      } else {
        yield promiseTimeOut();
      }
    }

    return code;
  }),

  






  _promiseTokenSet: function(code) {
    return new Promise(function(resolve, reject) {
      let request = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
                      .createInstance(Ci.nsIXMLHttpRequest);

      request.open("POST", getUrlParam("https://accounts.google.com/o/oauth2/",
                                       "loop.oauth.google.URL",
                                       false) + "token");

      request.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");

      request.onload = function() {
        if (request.status < 400) {
          let tokenSet = JSON.parse(request.responseText);
          tokenSet.date = Date.now();
          resolve(tokenSet);
        } else {
          reject(new Error(request.status + " " + request.statusText));
        }
      };

      request.onerror = function(error) {
        reject(error);
      };

      let body = "grant_type=authorization_code&code=" + encodeURIComponent(code) +
                 "&client_id=" + getUrlParam("%GOOGLE_OAUTH_API_CLIENTID%",
                                             "loop.oauth.google.clientIdOverride") +
                 "&client_secret=" + getUrlParam("%GOOGLE_OAUTH_API_KEY%",
                                                 "loop.oauth.google.clientSecretOverride") +
                 "&redirect_uri=" + encodeURIComponent(Services.prefs.getCharPref(
                                                       "loop.oauth.google.redirect_uri"));

      request.send(body);
    });
  },

  _promiseRequestXML: function(URL, tokenSet) {
    return new Promise((resolve, reject) => {
      let request = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
                      .createInstance(Ci.nsIXMLHttpRequest);

      request.open("GET", URL);

      request.setRequestHeader("Content-Type", "application/xml; charset=utf-8");
      request.setRequestHeader("GData-Version", "3.0");
      request.setRequestHeader("Authorization", "Bearer " + tokenSet.access_token);

      request.onload = function() {
        if (request.status < 400) {
          let doc = request.responseXML;
          
          let currNode = doc.documentElement.firstChild;
          while (currNode) {
            if (currNode.nodeType == 1 && currNode.localName == "id") {
              gProfileId = currNode.textContent;
              break;
            }
            currNode = currNode.nextSibling;
          }

          resolve(doc);
        } else {
          reject(new Error(request.status + " " + request.statusText));
        }
      };

      request.onerror = function(error) {
        reject(error);
      }

      request.send();
    });
  },

  







  _getContactEntries: Task.async(function* (tokenSet) {
    let URL = getUrlParam("https://www.google.com/m8/feeds/contacts/default/full",
                          "loop.oauth.google.getContactsURL",
                          false) + "?max-results=" + kContactsMaxResults;
    let xmlDoc = yield this._promiseRequestXML(URL, tokenSet);
    
    return Array.prototype.slice.call(xmlDoc.querySelectorAll("entry"));
  }),

  







  _getContactsGroupId: Task.async(function* (tokenSet) {
    let URL = getUrlParam("https://www.google.com/m8/feeds/groups/default/full",
                          "loop.oauth.google.getGroupsURL",
                          false) + "?max-results=" + kContactsMaxResults;
    let xmlDoc = yield this._promiseRequestXML(URL, tokenSet);
    let contactsEntry = xmlDoc.querySelector("systemGroup[id=\"Contacts\"]");
    if (!contactsEntry) {
      throw new Error("Contacts group not present");
    }
    
    
    contactsEntry = contactsEntry.parentNode;
    return contactsEntry.getElementsByTagName("id")[0].textContent;
  }),

  


















  _processContacts: Task.async(function* (contactEntries, db, tokenSet) {
    let stats = {
      total: contactEntries.length,
      success: 0,
      ids: {}
    };

    
    let contactsGroupId = yield this._getContactsGroupId(tokenSet);

    for (let entry of contactEntries) {
      let contact = this._processContactFields(entry);

      stats.ids[contact.id] = 1;
      let existing = yield db.promise("getByServiceId", contact.id);
      if (existing) {
        yield db.promise("remove", existing._guid);
      }

      
      if (!entry.querySelector("groupMembershipInfo[deleted=\"false\"][href=\"" +
                               contactsGroupId + "\"]")) {
        continue;
      }

      
      
      if (!("email" in contact) && !("tel" in contact)) {
        continue;
      }

      yield db.promise("add", contact);
      stats.success++;
    }

    return stats;
  }),

  






  _processContactFields: function(entry) {
    
    let contact = extractFieldsFromNode(new Map([
      ["id", "id"],
      
      ["updated", "updated"]
      
    ]), entry);

    
    extractFieldsFromNode(new Map([
      ["name", "fullName"],
      ["givenName", "givenName"],
      ["familyName", "familyName"],
      ["additionalName", "additionalName"]
    ]), entry, kNS_GD, contact, true);

    
    
    extractFieldsFromNode(new Map([
      ["note", "content"]
    ]), entry, null, contact, true);

    
    let addressNodes = entry.getElementsByTagNameNS(kNS_GD, "structuredPostalAddress");
    if (addressNodes.length) {
      contact.adr = [];
      for (let [,addressNode] of Iterator(addressNodes)) {
        let adr = extractFieldsFromNode(new Map([
          ["countryName", "country"],
          ["locality", "city"],
          ["postalCode", "postcode"],
          ["region", "region"],
          ["streetAddress", "street"]
        ]), addressNode, kNS_GD);
        if (Object.keys(adr).length) {
          adr.pref = (addressNode.getAttribute("primary") == "true");
          adr.type = [getFieldType(addressNode)];
          contact.adr.push(adr);
        }
      }
    }

    
    let emailNodes = entry.getElementsByTagNameNS(kNS_GD, "email");
    if (emailNodes.length) {
      contact.email = [];
      for (let [,emailNode] of Iterator(emailNodes)) {
        contact.email.push({
          pref: (emailNode.getAttribute("primary") == "true"),
          type: [getFieldType(emailNode)],
          value: emailNode.getAttribute("address")
        });
      }
    }

    
    let phoneNodes = entry.getElementsByTagNameNS(kNS_GD, "phoneNumber");
    if (phoneNodes.length) {
      contact.tel = [];
      for (let [,phoneNode] of Iterator(phoneNodes)) {
        let phoneNumber = phoneNode.hasAttribute("uri") ?
          phoneNode.getAttribute("uri").replace("tel:", "") :
          phoneNode.textContent;
        contact.tel.push({
          pref: (phoneNode.getAttribute("primary") == "true"),
          type: [getFieldType(phoneNode)],
          value: phoneNumber
        });
      }
    }

    let orgNodes = entry.getElementsByTagNameNS(kNS_GD, "organization");
    if (orgNodes.length) {
      contact.org = [];
      contact.jobTitle = [];
      for (let [,orgNode] of Iterator(orgNodes)) {
        let orgElement = orgNode.getElementsByTagNameNS(kNS_GD, "orgName")[0];
        let titleElement = orgNode.getElementsByTagNameNS(kNS_GD, "orgTitle")[0];
        contact.org.push(orgElement ? orgElement.textContent : "")
        contact.jobTitle.push(titleElement ? titleElement.textContent : "");
      }
    }

    contact.category = ["google"];

    
    if (!("name" in contact) || contact.name[0].length == 0) {
      if (("familyName" in contact) && ("givenName" in contact)) {
        
        
        
        
        contact.name = [contact.familyName[0] + ", " + contact.givenName[0]];
        if (("additionalName" in contact)) {
          contact.name[0] += " " + contact.additionalName[0];
        }
      } else {
        let profileTitle = extractFieldsFromNode(new Map([["title", "title"]]), entry);
        if (("title" in profileTitle)) {
          contact.name = [profileTitle.title];
        } else if ("familyName" in contact) {
          contact.name = [contact.familyName[0]];
        } else if ("givenName" in contact) {
          contact.name = [contact.givenName[0]];
        } else if ("org" in contact) {
          contact.name = [contact.org[0]];
        } else {
          let email;
          try {
            email = getPreferred(contact);
          } catch (ex) {}
          if (email) {
            contact.name = [email.value];
          } else {
            let tel;
            try {
              tel = getPreferred(contact, "tel");
            } catch (ex) {}
            if (tel) {
              contact.name = [tel.value];
            }
          }
        }
      }
    }

    return contact;
  },

  








  _purgeContacts: Task.async(function* (ids, db) {
    let contacts = yield db.promise("getAll");
    let profileId = "https://www.google.com/m8/feeds/contacts/" + encodeURIComponent(gProfileId);
    let processed = 0;

    function promiseSkipABeat() {
      return new Promise(resolve => Services.tm.currentThread.dispatch(resolve,
                                      Ci.nsIThread.DISPATCH_NORMAL));
    }

    for (let [guid, contact] of Iterator(contacts)) {
      if (++processed % kContactsChunkSize === 0) {
        
        yield promiseSkipABeat;
      }

      if (contact.id.indexOf(profileId) >= 0 && !ids[contact.id]) {
        yield db.promise("remove", guid);
      }
    }
  })
};
