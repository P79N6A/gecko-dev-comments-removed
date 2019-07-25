




































const EXPORTED_SYMBOLS = ['Weave'];

let Weave = {};
Components.utils.import("resource://services-sync/constants.js", Weave);
let lazies = {
  "record.js":            ["CollectionKeys", "BulkKeyBundle", "SyncKeyBundle"],
  "engines.js":           ['Engines', 'Engine', 'SyncEngine', 'Store'],
  "engines/bookmarks.js": ['BookmarksEngine', 'BookmarksSharingManager'],
  "engines/clients.js":   ["Clients"],
  "engines/forms.js":     ["FormEngine"],
  "engines/history.js":   ["HistoryEngine"],
  "engines/prefs.js":     ["PrefsEngine"],
  "engines/passwords.js": ["PasswordEngine"],
  "engines/tabs.js":      ["TabEngine"],
  "identity.js":          ["Identity", "ID"],
  "jpakeclient.js":       ["JPAKEClient"],
  "notifications.js":     ["Notifications", "Notification", "NotificationButton"],
  "policies.js":          ["SyncScheduler", "ErrorHandler",
                           "SendCredentialsController"],
  "resource.js":          ["Resource", "AsyncResource", "Auth",
                           "BasicAuthenticator", "NoOpAuthenticator"],
  "service.js":           ["Service"],
  "status.js":            ["Status"],
  "util.js":              ['Utils', 'Svc', 'Str']
};

function lazyImport(module, dest, props) {
  function getter(prop) function() {
    let ns = {};
    Components.utils.import(module, ns);
    delete dest[prop];
    return dest[prop] = ns[prop];
  };
  props.forEach(function(prop) dest.__defineGetter__(prop, getter(prop)));
}

for (let mod in lazies) {
  lazyImport("resource://services-sync/" + mod, Weave, lazies[mod]);
}
