



this.EXPORTED_SYMBOLS = ['Weave'];

this.Weave = {};
Components.utils.import("resource://services-sync/constants.js", Weave);
let lazies = {
  "jpakeclient.js":       ["JPAKEClient", "SendCredentialsController"],
  "notifications.js":     ["Notifications", "Notification", "NotificationButton"],
  "service.js":           ["Service"],
  "status.js":            ["Status"],
  "util.js":              ['Utils', 'Svc']
};

function lazyImport(module, dest, props) {
  function getter(prop) function() {
    let ns = {};
    Components.utils.import(module, ns);
    delete dest[prop];
    return dest[prop] = ns[prop];
  };
  props.forEach(function (prop) { dest.__defineGetter__(prop, getter(prop)); });
}

for (let mod in lazies) {
  lazyImport("resource://services-sync/" + mod, Weave, lazies[mod]);
}
