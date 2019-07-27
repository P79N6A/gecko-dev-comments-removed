

























package ch.boye.httpclientandroidlib.impl.client;






class SystemClock implements Clock {

    public long getCurrentTime() {
        return System.currentTimeMillis();
    }

}
