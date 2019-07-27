

if (typeof IdentityService !== "undefined") {
  IdentityService.shutdown();
} else if (typeof IDService !== "undefined") {
  IDService.shutdown();
}

