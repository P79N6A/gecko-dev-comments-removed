function check_ip(s, v, ip) {
  do_check_false(s.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS, ip, 0));

  let str = "https://";
  if (v == 6) {
    str += "[";
  }
  str += ip;
  if (v == 6) {
    str += "]";
  }
  str += "/";

  let uri = Services.io.newURI(str, null, null);

  let parsedMaxAge = {};
  let parsedIncludeSubdomains = {};
  s.processHeader(Ci.nsISiteSecurityService.HEADER_HSTS, uri,
                  "max-age=1000;includeSubdomains", 0,
                  parsedMaxAge, parsedIncludeSubdomains);

  




  do_check_neq(parsedMaxAge.value, 1000);
  do_check_neq(parsedIncludeSubdomains.value, true);
}

function run_test() {
  let SSService = Cc["@mozilla.org/ssservice;1"]
                    .getService(Ci.nsISiteSecurityService);

  check_ip(SSService, 4, "127.0.0.1");
  check_ip(SSService, 4, "10.0.0.1");
  check_ip(SSService, 6, "2001:db8::1");
  check_ip(SSService, 6, "1080::8:800:200C:417A");
}
