


























package ch.boye.httpclientandroidlib.io;

import java.io.IOException;

import ch.boye.httpclientandroidlib.util.CharArrayBuffer;










public interface SessionOutputBuffer {

    












    void write(byte[] b, int off, int len) throws IOException;

    






    void write(byte[] b) throws IOException;

    





    void write(int b) throws IOException;

    









    void writeLine(String s) throws IOException;

    









    void writeLine(CharArrayBuffer buffer) throws IOException;

    









    void flush() throws IOException;

    




    HttpTransportMetrics getMetrics();

}
