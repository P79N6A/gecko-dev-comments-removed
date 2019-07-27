




package org.mozilla.gecko.util;

import android.util.Log;

import java.io.IOException;
import java.io.InputStream;




public class IOUtils {
    private static final String LOGTAG = "GeckoIOUtils";

    






    public static class ConsumedInputStream {
        public final int consumedLength;
        
        private byte[] consumedData;

        public ConsumedInputStream(int consumedLength, byte[] consumedData) {
            this.consumedLength = consumedLength;
            this.consumedData = consumedData;
        }

        


        public byte[] getTruncatedData() {
            if (consumedData.length == consumedLength) {
                return consumedData;
            }

            consumedData = truncateBytes(consumedData, consumedLength);
            return consumedData;
        }

        public byte[] getData() {
            return consumedData;
        }
    }

    






    public static ConsumedInputStream readFully(InputStream iStream, int bufferSize) {
        
        byte[] buffer = new byte[bufferSize];

        
        int bPointer = 0;

        
        int lastRead = 0;
        try {
            
            while (lastRead != -1) {
                
                lastRead = iStream.read(buffer, bPointer, buffer.length - bPointer);
                bPointer += lastRead;

                
                if (bPointer == buffer.length) {
                    bufferSize *= 2;
                    byte[] newBuffer = new byte[bufferSize];

                    
                    System.arraycopy(buffer, 0, newBuffer, 0, buffer.length);
                    buffer = newBuffer;
                }
            }

            return new ConsumedInputStream(bPointer + 1, buffer);
        } catch (IOException e) {
            Log.e(LOGTAG, "Error consuming input stream.", e);
        } finally {
            try {
                iStream.close();
            } catch (IOException e) {
                Log.e(LOGTAG, "Error closing input stream.", e);
            }
        }

        return null;
    }

    



    public static byte[] truncateBytes(byte[] bytes, int length) {
        byte[] newBytes = new byte[length];
        System.arraycopy(bytes, 0, newBytes, 0, length);

        return newBytes;
    }
}
