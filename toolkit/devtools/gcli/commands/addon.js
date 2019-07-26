



"use strict";

const { Cc, Ci, Cu } = require("chrome");
const { AddonManager } = Cu.import("resource://gre/modules/AddonManager.jsm", {});
const gcli = require("gcli/index");
const { Promise: promise } = require("resource://gre/modules/Promise.jsm");

const BRAND_SHORT_NAME = Cc["@mozilla.org/intl/stringbundle;1"]
                           .getService(Ci.nsIStringBundleService)
                           .createBundle("chrome://branding/locale/brand.properties")
                           .GetStringFromName("brandShortName");






const promiseify = function(scope, functionWithLastParamCallback) {
  return (...args) => {
    return new Promise(resolve => {
      args.push((...results) => {
        resolve(results.length > 1 ? results : results[0]);
      });
      functionWithLastParamCallback.apply(scope, args);
    });
  }
};


const getAllAddons = promiseify(AddonManager, AddonManager.getAllAddons);
const getAddonsByTypes = promiseify(AddonManager, AddonManager.getAddonsByTypes);




function pendingOperations(addon) {
  let allOperations = [
    "PENDING_ENABLE", "PENDING_DISABLE", "PENDING_UNINSTALL",
    "PENDING_INSTALL", "PENDING_UPGRADE"
  ];
  return allOperations.reduce(function(operations, opName) {
    return addon.pendingOperations & AddonManager[opName] ?
      operations.concat(opName) :
      operations;
  }, []);
}

exports.items = [
  {
    item: "type",
    name: "addon",
    parent: "selection",
    stringifyProperty: "name",
    cacheable: true,
    constructor: function() {
      
      let listener = {
        onInstalled: addon => { this.clearCache(); },
        onUninstalled: addon => { this.clearCache(); },
      };
      AddonManager.addAddonListener(listener);
    },
    lookup: function() {
      return getAllAddons().then(addons => {
        return addons.map(addon => {
          let name = addon.name + " " + addon.version;
          name = name.trim().replace(/\s/g, "_");
          return { name: name, value: addon };
        });
      });
    }
  },
  {
    name: "addon",
    description: gcli.lookup("addonDesc")
  },
  {
    name: "addon list",
    description: gcli.lookup("addonListDesc"),
    returnType: "addonsInfo",
    params: [{
      name: "type",
      type: {
        name: "selection",
        data: [ "dictionary", "extension", "locale", "plugin", "theme", "all" ]
      },
      defaultValue: "all",
      description: gcli.lookup("addonListTypeDesc")
    }],
    exec: function(args, context) {
      let types = (args.type === "all") ? null : [ args.type ];
      return getAddonsByTypes(types).then(addons => {
        addons = addons.map(function(addon) {
          return {
            name: addon.name,
            version: addon.version,
            isActive: addon.isActive,
            pendingOperations: pendingOperations(addon)
          };
        });
        return { addons: addons, type: args.type };
      });
    }
  },
  {
    item: "converter",
    from: "addonsInfo",
    to: "view",
    exec: function(addonsInfo, context) {
      if (!addonsInfo.addons.length) {
        return context.createView({
          html: "<p>${message}</p>",
          data: { message: gcli.lookup("addonNoneOfType") }
        });
      }

      let headerLookups = {
        "dictionary": "addonListDictionaryHeading",
        "extension": "addonListExtensionHeading",
        "locale": "addonListLocaleHeading",
        "plugin": "addonListPluginHeading",
        "theme": "addonListThemeHeading",
        "all": "addonListAllHeading"
      };
      let header = gcli.lookup(headerLookups[addonsInfo.type] ||
                               "addonListUnknownHeading");

      let operationLookups = {
        "PENDING_ENABLE": "addonPendingEnable",
        "PENDING_DISABLE": "addonPendingDisable",
        "PENDING_UNINSTALL": "addonPendingUninstall",
        "PENDING_INSTALL": "addonPendingInstall",
        "PENDING_UPGRADE": "addonPendingUpgrade"
      };
      function lookupOperation(opName) {
        let lookupName = operationLookups[opName];
        return lookupName ? gcli.lookup(lookupName) : opName;
      }

      function arrangeAddons(addons) {
        let enabledAddons = [];
        let disabledAddons = [];
        addons.forEach(function(addon) {
          if (addon.isActive) {
            enabledAddons.push(addon);
          } else {
            disabledAddons.push(addon);
          }
        });

        function compareAddonNames(nameA, nameB) {
          return String.localeCompare(nameA.name, nameB.name);
        }
        enabledAddons.sort(compareAddonNames);
        disabledAddons.sort(compareAddonNames);

        return enabledAddons.concat(disabledAddons);
      }

      function isActiveForToggle(addon) {
        return (addon.isActive && ~~addon.pendingOperations.indexOf("PENDING_DISABLE"));
      }

      return context.createView({
        html:
          "<table>" +
          " <caption>${header}</caption>" +
          " <tbody>" +
          "  <tr foreach='addon in ${addons}'" +
          "      class=\"gcli-addon-${addon.status}\">" +
          "    <td>${addon.name} ${addon.version}</td>" +
          "    <td>${addon.pendingOperations}</td>" +
          "    <td>" +
          "      <span class='gcli-out-shortcut'" +
          "          data-command='addon ${addon.toggleActionName} ${addon.label}'" +
          "          onclick='${onclick}' ondblclick='${ondblclick}'" +
          "      >${addon.toggleActionMessage}</span>" +
          "    </td>" +
          "  </tr>" +
          " </tbody>" +
          "</table>",
        data: {
          header: header,
          addons: arrangeAddons(addonsInfo.addons).map(function(addon) {
            return {
              name: addon.name,
              label: addon.name.replace(/\s/g, "_") +
                    (addon.version ? "_" + addon.version : ""),
              status: addon.isActive ? "enabled" : "disabled",
              version: addon.version,
              pendingOperations: addon.pendingOperations.length ?
                (" (" + gcli.lookup("addonPending") + ": "
                 + addon.pendingOperations.map(lookupOperation).join(", ")
                 + ")") :
                "",
              toggleActionName: isActiveForToggle(addon) ? "disable": "enable",
              toggleActionMessage: isActiveForToggle(addon) ?
                gcli.lookup("addonListOutDisable") :
                gcli.lookup("addonListOutEnable")
            };
          }),
          onclick: context.update,
          ondblclick: context.updateExec
        }
      });
    }
  },
  {
    name: "addon enable",
    description: gcli.lookup("addonEnableDesc"),
    params: [
      {
        name: "addon",
        type: "addon",
        description: gcli.lookup("addonNameDesc")
      }
    ],
    exec: function(args, context) {
      let name = (args.addon.name + " " + args.addon.version).trim();
      if (args.addon.userDisabled) {
        args.addon.userDisabled = false;
        return gcli.lookupFormat("addonEnabled", [ name ]);
      }

      return gcli.lookupFormat("addonAlreadyEnabled", [ name ]);
    }
  },
  {
    name: "addon disable",
    description: gcli.lookup("addonDisableDesc"),
    params: [
      {
        name: "addon",
        type: "addon",
        description: gcli.lookup("addonNameDesc")
      }
    ],
    exec: function(args, context) {
      
      
      
      let name = (args.addon.name + " " + args.addon.version).trim();
      if (!args.addon.userDisabled ||
          args.addon.userDisabled === AddonManager.STATE_ASK_TO_ACTIVATE) {
        args.addon.userDisabled = true;
        return gcli.lookupFormat("addonDisabled", [ name ]);
      }

      return gcli.lookupFormat("addonAlreadyDisabled", [ name ]);
    }
  }
];
