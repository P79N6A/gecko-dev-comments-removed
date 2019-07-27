



package org.mozilla.gecko;

import org.mozilla.gecko.distribution.ReferrerDescriptor;

public class TestDistribution extends BrowserTestCase {
    private static final String TEST_REFERRER_STRING = "utm_source=campsource&utm_medium=campmed&utm_term=term%2Bhere&utm_content=content&utm_campaign=name";
    private static final String TEST_MALFORMED_REFERRER_STRING = "utm_source=campsource&utm_medium=campmed&utm_term=term%2";

    public void testReferrerParsing() {
        ReferrerDescriptor good = new ReferrerDescriptor(TEST_REFERRER_STRING);
        assertEquals("campsource", good.source);
        assertEquals("campmed", good.medium);
        assertEquals("term+here", good.term);
        assertEquals("content", good.content);
        assertEquals("name", good.campaign);

        
        ReferrerDescriptor bad = new ReferrerDescriptor(TEST_MALFORMED_REFERRER_STRING);
        assertEquals("campsource", bad.source);
        assertEquals("campmed", bad.medium);
        assertFalse("term+here".equals(bad.term));
        assertNull(bad.content);
        assertNull(bad.campaign);

        ReferrerDescriptor ugly = new ReferrerDescriptor(null);
        assertNull(ugly.source);
        assertNull(ugly.medium);
        assertNull(ugly.term);
        assertNull(ugly.content);
        assertNull(ugly.campaign);
    }
}
