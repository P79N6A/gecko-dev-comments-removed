



package org.mozilla.gecko.favicons.decoders;

import android.graphics.Bitmap;
import org.mozilla.gecko.favicons.Favicons;
import org.mozilla.gecko.gfx.BitmapUtils;

import android.util.SparseArray;

import java.util.Iterator;
import java.util.NoSuchElementException;

























































public class ICODecoder implements Iterable<Bitmap> {
    
    public static final int COMPACT_THRESHOLD = 4000;

    
    public static final int ICO_HEADER_LENGTH_BYTES = 6;
    public static final int ICO_ICONDIRENTRY_LENGTH_BYTES = 16;

    
    private byte[] decodand;

    
    private int offset;
    private int len;

    IconDirectoryEntry[] iconDirectory;
    private boolean isValid;
    private boolean hasDecoded;

    public ICODecoder(byte[] decodand, int offset, int len) {
        this.decodand = decodand;
        this.offset = offset;
        this.len = len;
    }

    





    private boolean decodeIconDirectoryAndPossiblyPrune() {
        hasDecoded = true;

        
        if (offset + len > decodand.length) {
            return false;
        }

        
        if (len < ICO_HEADER_LENGTH_BYTES) {
            return false;
        }

        
        
        if (decodand[offset] != 0 ||
            decodand[offset + 1] != 0 ||
            decodand[offset + 2] != 1 ||
            decodand[offset + 3] != 0) {
            return false;
        }

        
        
        
        int numEncodedImages = (decodand[offset + 4] & 0xFF) |
                               (decodand[offset + 5] & 0xFF) << 8;


        
        if (numEncodedImages <= 0) {
            return false;
        }

        final int headerAndDirectorySize = ICO_HEADER_LENGTH_BYTES + (numEncodedImages * ICO_ICONDIRENTRY_LENGTH_BYTES);

        
        
        if (len < headerAndDirectorySize) {
            return false;
        }

        
        int bufferIndex = offset + ICO_HEADER_LENGTH_BYTES;

        
        

        
        int minimumMaximum = Integer.MAX_VALUE;

        
        SparseArray<IconDirectoryEntry> preferenceArray = new SparseArray<IconDirectoryEntry>();

        for (int i = 0; i < numEncodedImages; i++, bufferIndex += ICO_ICONDIRENTRY_LENGTH_BYTES) {
            
            IconDirectoryEntry newEntry = IconDirectoryEntry.createFromBuffer(decodand, offset, len, bufferIndex);
            newEntry.index = i;

            if (newEntry.isErroneous) {
                continue;
            }

            if (newEntry.width > Favicons.largestFaviconSize) {
                
                
                
                if (newEntry.width >= minimumMaximum) {
                    continue;
                }

                
                preferenceArray.delete(minimumMaximum);

                minimumMaximum = newEntry.width;
            }

            IconDirectoryEntry oldEntry = preferenceArray.get(newEntry.width);
            if (oldEntry == null) {
                preferenceArray.put(newEntry.width, newEntry);
                continue;
            }

            if (oldEntry.compareTo(newEntry) < 0) {
                preferenceArray.put(newEntry.width, newEntry);
            }
        }

        final int count = preferenceArray.size();

        
        if (count == 0) {
            return false;
        }

        
        iconDirectory = new IconDirectoryEntry[count];

        
        int retainedSpace = ICO_HEADER_LENGTH_BYTES;

        for (int i = 0; i < count; i++) {
            IconDirectoryEntry e = preferenceArray.valueAt(i);
            retainedSpace += ICO_ICONDIRENTRY_LENGTH_BYTES + e.payloadSize;
            iconDirectory[i] = e;
        }

        isValid = true;

        
        decodand[offset + 4] = (byte) iconDirectory.length;
        decodand[offset + 5] = (byte) (iconDirectory.length >>> 8);

        if ((len - retainedSpace) > COMPACT_THRESHOLD) {
            compactingCopy(retainedSpace);
        }

        return true;
    }

    


    private void compactingCopy(int spaceRetained) {
        byte[] buf = new byte[spaceRetained];

        
        System.arraycopy(decodand, offset, buf, 0, ICO_HEADER_LENGTH_BYTES);

        int headerPtr = ICO_HEADER_LENGTH_BYTES;

        int payloadPtr = ICO_HEADER_LENGTH_BYTES + (iconDirectory.length * ICO_ICONDIRENTRY_LENGTH_BYTES);

        int ind = 0;
        for (IconDirectoryEntry entry : iconDirectory) {
            
            System.arraycopy(decodand, offset + entry.getOffset(), buf, headerPtr, ICO_ICONDIRENTRY_LENGTH_BYTES);

            
            System.arraycopy(decodand, offset + entry.payloadOffset, buf, payloadPtr, entry.payloadSize);

            
            buf[headerPtr + 12] = (byte) payloadPtr;
            buf[headerPtr + 13] = (byte) (payloadPtr >>> 8);
            buf[headerPtr + 14] = (byte) (payloadPtr >>> 16);
            buf[headerPtr + 15] = (byte) (payloadPtr >>> 24);

            entry.payloadOffset = payloadPtr;
            entry.index = ind;

            payloadPtr += entry.payloadSize;
            headerPtr += ICO_ICONDIRENTRY_LENGTH_BYTES;
            ind++;
        }

        decodand = buf;
        offset = 0;
        len = spaceRetained;
    }

    






    public Bitmap decodeBitmapAtIndex(int index) {
        final IconDirectoryEntry iconDirEntry = iconDirectory[index];

        if (iconDirEntry.payloadIsPNG) {
            
            return BitmapUtils.decodeByteArray(decodand, offset + iconDirEntry.payloadOffset, iconDirEntry.payloadSize);
        }

        
        
        byte[] decodeTarget = new byte[ICO_HEADER_LENGTH_BYTES + ICO_ICONDIRENTRY_LENGTH_BYTES + iconDirEntry.payloadSize];

        
        decodeTarget[2] = 1;

        
        decodeTarget[4] = 1;

        
        System.arraycopy(decodand, offset + iconDirEntry.getOffset(), decodeTarget, ICO_HEADER_LENGTH_BYTES, ICO_ICONDIRENTRY_LENGTH_BYTES);

        
        final int singlePayloadOffset =  ICO_HEADER_LENGTH_BYTES + ICO_ICONDIRENTRY_LENGTH_BYTES;
        System.arraycopy(decodand, offset + iconDirEntry.payloadOffset, decodeTarget, singlePayloadOffset, iconDirEntry.payloadSize);

        
        decodeTarget[ICO_HEADER_LENGTH_BYTES + 12] = (byte) singlePayloadOffset;
        decodeTarget[ICO_HEADER_LENGTH_BYTES + 13] = (byte) (singlePayloadOffset >>> 8);
        decodeTarget[ICO_HEADER_LENGTH_BYTES + 14] = (byte) (singlePayloadOffset >>> 16);
        decodeTarget[ICO_HEADER_LENGTH_BYTES + 15] = (byte) (singlePayloadOffset >>> 24);

        
        return BitmapUtils.decodeByteArray(decodeTarget);
    }

    




    @Override
    public ICOIterator iterator() {
        
        if (hasDecoded && !isValid) {
            return null;
        }

        
        if (!hasDecoded) {
            if (!decodeIconDirectoryAndPossiblyPrune()) {
                return null;
            }
        }

        
        return new ICOIterator();
    }

    



    public LoadFaviconResult decode() {
        
        Iterator<Bitmap> bitmaps = iterator();
        if (bitmaps == null) {
            return null;
        }

        LoadFaviconResult result = new LoadFaviconResult();

        result.bitmapsDecoded = bitmaps;
        result.faviconBytes = decodand;
        result.offset = offset;
        result.length = len;
        result.isICO = true;

        return result;
    }

    


    private class ICOIterator implements Iterator<Bitmap> {
        private int mIndex;

        @Override
        public boolean hasNext() {
            return mIndex < iconDirectory.length;
        }

        @Override
        public Bitmap next() {
            if (mIndex > iconDirectory.length) {
                throw new NoSuchElementException("No more elements in this ICO.");
            }
            return decodeBitmapAtIndex(mIndex++);
        }

        @Override
        public void remove() {
            if (iconDirectory[mIndex] == null) {
                throw new IllegalStateException("Remove already called for element " + mIndex);
            }
            iconDirectory[mIndex] = null;
        }
    }
}
