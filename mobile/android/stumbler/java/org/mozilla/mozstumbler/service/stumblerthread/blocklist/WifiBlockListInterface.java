



package org.mozilla.mozstumbler.service.stumblerthread.blocklist;

public interface WifiBlockListInterface {
    String[] getSsidPrefixList();
    String[] getSsidSuffixList();
    String[] getBssidOuiList();
}
