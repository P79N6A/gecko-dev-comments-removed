


Cu.import("resource://services-common/utils.js");

function run_test() {
  let data = ["Zm9vYmE=", "Zm9vYmE==", "Zm9vYmE==="];
  for (let d in data) {
    do_check_eq(CommonUtils.safeAtoB(data[d]), "fooba");
  }
}
