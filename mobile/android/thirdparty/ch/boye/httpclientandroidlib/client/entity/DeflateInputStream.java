

























package ch.boye.httpclientandroidlib.client.entity;

import java.io.IOException;
import java.io.InputStream;
import java.io.PushbackInputStream;
import java.util.zip.DataFormatException;
import java.util.zip.Inflater;
import java.util.zip.InflaterInputStream;




public class DeflateInputStream extends InputStream
{
    private InputStream sourceStream;

    public DeflateInputStream(final InputStream wrapped)
        throws IOException
    {
        
























        
        final byte[] peeked = new byte[6];

        final PushbackInputStream pushback = new PushbackInputStream(wrapped, peeked.length);

        final int headerLength = pushback.read(peeked);

        if (headerLength == -1) {
            throw new IOException("Unable to read the response");
        }

        
        final byte[] dummy = new byte[1];

        final Inflater inf = new Inflater();

        try {
            int n;
            while ((n = inf.inflate(dummy)) == 0) {
                if (inf.finished()) {

                    
                    throw new IOException("Unable to read the response");
                }

                if (inf.needsDictionary()) {

                    
                    break;
                }

                if (inf.needsInput()) {
                    inf.setInput(peeked);
                }
            }

            if (n == -1) {
                throw new IOException("Unable to read the response");
            }

            



            pushback.unread(peeked, 0, headerLength);
            sourceStream = new DeflateStream(pushback, new Inflater());
        } catch (final DataFormatException e) {

            

            pushback.unread(peeked, 0, headerLength);
            sourceStream = new DeflateStream(pushback, new Inflater(true));
        } finally {
            inf.end();
        }

    }

    

    @Override
    public int read()
        throws IOException
    {
        return sourceStream.read();
    }

    

    @Override
    public int read(final byte[] b)
        throws IOException
    {
        return sourceStream.read(b);
    }

    

    @Override
    public int read(final byte[] b, final int off, final int len)
        throws IOException
    {
        return sourceStream.read(b,off,len);
    }

    

    @Override
    public long skip(final long n)
        throws IOException
    {
        return sourceStream.skip(n);
    }

    

    @Override
    public int available()
        throws IOException
    {
        return sourceStream.available();
    }

    

    @Override
    public void mark(final int readLimit)
    {
        sourceStream.mark(readLimit);
    }

    

    @Override
    public void reset()
        throws IOException
    {
        sourceStream.reset();
    }

    

    @Override
    public boolean markSupported()
    {
        return sourceStream.markSupported();
    }

    

    @Override
    public void close()
        throws IOException
    {
        sourceStream.close();
    }

    static class DeflateStream extends InflaterInputStream {

        private boolean closed = false;

        public DeflateStream(final InputStream in, final Inflater inflater) {
            super(in, inflater);
        }

        @Override
        public void close() throws IOException {
            if (closed) {
                return;
            }
            closed = true;
            inf.end();
            super.close();
        }

    }

}

