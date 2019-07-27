

























package ch.boye.httpclientandroidlib.impl.client.cache;

import java.net.InetAddress;
import java.net.UnknownHostException;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.util.Formatter;
import java.util.Locale;

import ch.boye.httpclientandroidlib.annotation.GuardedBy;
import ch.boye.httpclientandroidlib.annotation.ThreadSafe;




@ThreadSafe
class BasicIdGenerator {

    private final String hostname;
    private final SecureRandom rnd;

    @GuardedBy("this")
    private long count;

    public BasicIdGenerator() {
        super();
        String hostname;
        try {
            hostname = InetAddress.getLocalHost().getHostName();
        } catch (final UnknownHostException ex) {
            hostname = "localhost";
        }
        this.hostname = hostname;
        try {
            this.rnd = SecureRandom.getInstance("SHA1PRNG");
        } catch (final NoSuchAlgorithmException ex) {
            throw new Error(ex);
        }
        this.rnd.setSeed(System.currentTimeMillis());
    }

    public synchronized void generate(final StringBuilder buffer) {
        this.count++;
        final int rndnum = this.rnd.nextInt();
        buffer.append(System.currentTimeMillis());
        buffer.append('.');
        final Formatter formatter = new Formatter(buffer, Locale.US);
        formatter.format("%1$016x-%2$08x", this.count, rndnum);
        formatter.close();
        buffer.append('.');
        buffer.append(this.hostname);
    }

    public String generate() {
        final StringBuilder buffer = new StringBuilder();
        generate(buffer);
        return buffer.toString();
    }

}
