




if (HAVE_TM) {
  assertEq(jitstats.archIsIA32 ||
	   jitstats.archIs64BIT ||
	   jitstats.archIsARM ||
	   jitstats.archIsSPARC ||
	   jitstats.archIsPPC ||
	   jitstats.archIsAMD64,
	   1);
 }
