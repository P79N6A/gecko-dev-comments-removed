



const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const IMIGRATOR = Ci.nsIBrowserProfileMigrator;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
                                  "resource://gre/modules/PlacesUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
                                  "resource://gre/modules/FileUtils.jsm");


let gProfD = do_get_profile();

function newMigratorFor(aKey) {
  let cid = "@mozilla.org/profile/migrator;1?app=browser&type=" + aKey;
  return Cc[cid].createInstance(Ci.nsIBrowserProfileMigrator);
}
