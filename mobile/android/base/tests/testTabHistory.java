package org.mozilla.gecko.tests;

public class testTabHistory extends BaseTest {

    @Override
    protected int getTestType() {
        return TEST_MOCHITEST;
    }

    




    public void testTabHistory() {
        blockForGeckoReady();

        String url = getAbsoluteUrl("/robocop/robocop_blank_01.html");
        String url2 = getAbsoluteUrl("/robocop/robocop_blank_02.html");
        String url3 = getAbsoluteUrl("/robocop/robocop_blank_03.html");

        
        inputAndLoadUrl(url);
        verifyPageTitle("Browser Blank Page 01");
        inputAndLoadUrl(url2);
        verifyPageTitle("Browser Blank Page 02");
        inputAndLoadUrl(url3);
        verifyPageTitle("Browser Blank Page 03");

        
        Navigation nav = new Navigation(mDevice);
        mAsserter.dumpLog("device type: "+mDevice.type);
        mAsserter.dumpLog("device version: "+mDevice.version);
        mAsserter.dumpLog("device width: "+mDevice.width);
        mAsserter.dumpLog("device height: "+mDevice.height);

        
        nav.back();
        waitForText("Browser Blank Page 02");
        verifyPageTitle("Browser Blank Page 02");

        
        nav.back();
        waitForText("Browser Blank Page 01");
        verifyPageTitle("Browser Blank Page 01");

        
        nav.forward();
        waitForText("Browser Blank Page 02");
        verifyPageTitle("Browser Blank Page 02");

        
        nav.reload();
        waitForText("Browser Blank Page 02");
        verifyPageTitle("Browser Blank Page 02");
    }
}
