



const modules = [
  "Identity.jsm",
  "IdentityProvider.jsm",
  "IdentityStore.jsm",
  "jwcrypto.jsm",
  "RelyingParty.jsm",
  "Sandbox.jsm",
];

function run_test() {
  for each (let m in modules) {
    let resource = "resource://gre/modules/identity/" + m;
    Components.utils.import(resource, {});
    do_print("loaded " + resource);
  }
}
