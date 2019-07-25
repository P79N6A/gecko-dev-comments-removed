




































const EXPORTED_SYMBOLS = ['Weave'];

let Weave = {};
Components.utils.import("resource://services-sync/constants.js", Weave);
let lazies = {
  "auth.js":              ['Auth', 'BrokenBasicAuthenticator',
                           'BasicAuthenticator', 'NoOpAuthenticator'],
  "base_records/crypto.js":
                          ["CollectionKeys", "BulkKeyBundle", "SyncKeyBundle"],
  "engines.js":           ['Engines', 'Engine', 'SyncEngine'],
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
  "resource.js":          ["Resource"],
  "service.js":           ["Service"],
  "status.js":            ["Status"],
  "stores.js":            ["Store"],
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
