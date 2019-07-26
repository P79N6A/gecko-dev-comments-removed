




Components.utils.import("resource://gre/modules/Preferences.jsm");
Components.utils.import("resource://gre/modules/UpdateChannel.jsm");

const PREF_APP_UPDATE_CHANNEL = "app.update.channel";
const TEST_CHANNEL            = "TestChannel";
const PREF_PARTNER_A          = "app.partner.test_partner_a";
const TEST_PARTNER_A          = "TestPartnerA";
const PREF_PARTNER_B          = "app.partner.test_partner_b";
const TEST_PARTNER_B          = "TestPartnerB";

function test_get() {
  let defaultPrefs = new Preferences({ defaultBranch: true });
  let currentChannel = defaultPrefs.get(PREF_APP_UPDATE_CHANNEL);

  do_check_eq(UpdateChannel.get(), currentChannel);
  do_check_eq(UpdateChannel.get(false), currentChannel);

  defaultPrefs.set(PREF_APP_UPDATE_CHANNEL, TEST_CHANNEL);
  do_check_eq(UpdateChannel.get(), TEST_CHANNEL);
  do_check_eq(UpdateChannel.get(false), TEST_CHANNEL);

  defaultPrefs.set(PREF_PARTNER_A, TEST_PARTNER_A);
  defaultPrefs.set(PREF_PARTNER_B, TEST_PARTNER_B);
  do_check_eq(UpdateChannel.get(),
              TEST_CHANNEL + "-cck-" + TEST_PARTNER_A + "-" + TEST_PARTNER_B);
  do_check_eq(UpdateChannel.get(false), TEST_CHANNEL);
}

function run_test() {
  test_get();
}
