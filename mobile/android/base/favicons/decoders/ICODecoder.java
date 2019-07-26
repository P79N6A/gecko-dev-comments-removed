



package org.mozilla.gecko.favicons.decoders;

import android.graphics.Bitmap;
import org.mozilla.gecko.favicons.Favicons;
import org.mozilla.gecko.gfx.BitmapUtils;

import java.util.Collection;
import java.util.HashMap;
import java.util.Iterator;
import java.util.NoSuchElementException;

























































public class ICODecoder implements Iterable<Bitmap> {
    
    public static final int COMPACT_THRESHOLD = 4000;

    
    public static final int ICO_HEADER_LENGTH_BYTES = 6;
    public static final int ICO_ICONDIRENTRY_LENGTH_BYTES = 16;

    
    private byte[] mDecodand;

    
    private int mOffset;
    private int mLen;

    private IconDirectoryEntry[] mIconDirectory;
    private boolean mIsValid;
    private boolean mHasDecoded;

    public ICODecoder(byte[] buffer, int offset, int len) {
        mDecodand = buffer;
        mOffset = offset;
        mLen = len;
    }

    





    private boolean decodeIconDirectoryAndPossiblyPrune() {
        mHasDecoded = true;

        
        if (mOffset + mLen > mDecodand.length) {
            return false;
        }

        
        if (mLen < ICO_HEADER_LENGTH_BYTES) {
            return false;
        }

        
        
        if (mDecodand[mOffset] != 0 ||
            mDecodand[mOffset + 1] != 0 ||
            mDecodand[mOffset + 2] != 1 ||
            mDecodand[mOffset + 3] != 0) {
            return false;
        }

        
        
        
        int numEncodedImages = (mDecodand[mOffset + 4] & 0xFF) |
                               (mDecodand[mOffset + 5] & 0xFF) << 8;


        
        if (numEncodedImages <= 0) {
            return false;
        }

        final int headerAndDirectorySize = ICO_HEADER_LENGTH_BYTES + (numEncodedImages * ICO_ICONDIRENTRY_LENGTH_BYTES);

        
        
        if (mLen < headerAndDirectorySize) {
            return false;
        }

        
        int bufferIndex = mOffset + ICO_HEADER_LENGTH_BYTES;

        
        

        
        int minimumMaximum = Integer.MAX_VALUE;

        
        HashMap<Integer, IconDirectoryEntry> preferenceMap = new HashMap<Integer, IconDirectoryEntry>();

        for (int i = 0; i < numEncodedImages; i++, bufferIndex += ICO_ICONDIRENTRY_LENGTH_BYTES) {
            
            IconDirectoryEntry newEntry = IconDirectoryEntry.createFromBuffer(mDecodand, mOffset, mLen, bufferIndex);
            newEntry.mIndex = i;

            if (newEntry.mIsErroneous) {
                continue;
            }

            if (newEntry.mWidth > Favicons.sLargestFaviconSize) {
                
                
                
                if (newEntry.mWidth >= minimumMaximum) {
                    continue;
                }

                
                if (preferenceMap.containsKey(minimumMaximum)) {
                    preferenceMap.remove(minimumMaximum);
                }

                minimumMaximum = newEntry.mWidth;
            }

            IconDirectoryEntry oldEntry = preferenceMap.get(newEntry.mWidth);
            if (oldEntry == null) {
                preferenceMap.put(newEntry.mWidth, newEntry);
                continue;
            }

            if (oldEntry.compareTo(newEntry) < 0) {
                preferenceMap.put(newEntry.mWidth, newEntry);
            }
        }

        Collection<IconDirectoryEntry> entriesRetained = preferenceMap.values();

        
        if (entriesRetained.isEmpty()) {
            return false;
        }

        
        mIconDirectory = new IconDirectoryEntry[entriesRetained.size()];

        
        int retainedSpace = ICO_HEADER_LENGTH_BYTES;

        int dirInd = 0;
        for (IconDirectoryEntry e : entriesRetained) {
            retainedSpace += ICO_ICONDIRENTRY_LENGTH_BYTES + e.mPayloadSize;
            mIconDirectory[dirInd] = e;
            dirInd++;
        }

        mIsValid = true;

        
        mDecodand[mOffset + 4] = (byte) mIconDirectory.length;
        mDecodand[mOffset + 5] = (byte) (mIconDirectory.length >>> 8);

        if ((mLen - retainedSpace) > COMPACT_THRESHOLD) {
            compactingCopy(retainedSpace);
        }

        return true;
    }

    


    private void compactingCopy(int spaceRetained) {
        byte[] buf = new byte[spaceRetained];

        
        System.arraycopy(mDecodand, mOffset, buf, 0, ICO_HEADER_LENGTH_BYTES);

        int headerPtr = ICO_HEADER_LENGTH_BYTES;

        int payloadPtr = ICO_HEADER_LENGTH_BYTES + (mIconDirectory.length * ICO_ICONDIRENTRY_LENGTH_BYTES);

        int ind = 0;
        for (IconDirectoryEntry entry : mIconDirectory) {
            
            System.arraycopy(mDecodand, mOffset + entry.getOffset(), buf, headerPtr, ICO_ICONDIRENTRY_LENGTH_BYTES);

            
            System.arraycopy(mDecodand, mOffset + entry.mPayloadOffset, buf, payloadPtr, entry.mPayloadSize);

            
            buf[headerPtr + 12] = (byte) payloadPtr;
            buf[headerPtr + 13] = (byte) (payloadPtr >>> 8);
            buf[headerPtr + 14] = (byte) (payloadPtr >>> 16);
            buf[headerPtr + 15] = (byte) (payloadPtr >>> 24);

            entry.mPayloadOffset = payloadPtr;
            entry.mIndex = ind;

            payloadPtr += entry.mPayloadSize;
            headerPtr += ICO_ICONDIRENTRY_LENGTH_BYTES;
            ind++;
        }

        mDecodand = buf;
        mOffset = 0;
        mLen = spaceRetained;
    }

    






    public Bitmap decodeBitmapAtIndex(int index) {
        final IconDirectoryEntry iconDirEntry = mIconDirectory[index];

        if (iconDirEntry.mPayloadIsPNG) {
            
            return BitmapUtils.decodeByteArray(mDecodand, mOffset + iconDirEntry.mPayloadOffset, iconDirEntry.mPayloadSize);
        }

        
        
        byte[] decodeTarget = new byte[ICO_HEADER_LENGTH_BYTES + ICO_ICONDIRENTRY_LENGTH_BYTES + iconDirEntry.mPayloadSize];

        
        decodeTarget[2] = 1;

        
        decodeTarget[4] = 1;

        
        System.arraycopy(mDecodand, mOffset + iconDirEntry.getOffset(), decodeTarget, ICO_HEADER_LENGTH_BYTES, ICO_ICONDIRENTRY_LENGTH_BYTES);

        
        final int singlePayloadOffset =  ICO_HEADER_LENGTH_BYTES + ICO_ICONDIRENTRY_LENGTH_BYTES;
        System.arraycopy(mDecodand, mOffset + iconDirEntry.mPayloadOffset, decodeTarget, singlePayloadOffset, iconDirEntry.mPayloadSize);

        
        decodeTarget[ICO_HEADER_LENGTH_BYTES + 12] = (byte) singlePayloadOffset;
        decodeTarget[ICO_HEADER_LENGTH_BYTES + 13] = (byte) (singlePayloadOffset >>> 8);
        decodeTarget[ICO_HEADER_LENGTH_BYTES + 14] = (byte) (singlePayloadOffset >>> 16);
        decodeTarget[ICO_HEADER_LENGTH_BYTES + 15] = (byte) (singlePayloadOffset >>> 24);

        
        return BitmapUtils.decodeByteArray(decodeTarget);
    }

    




    @Override
    public ICOIterator iterator() {
        
        if (mHasDecoded && !mIsValid) {
            return null;
        }

        
        if (!mHasDecoded) {
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

        result.mBitmapsDecoded = bitmaps;
        result.mFaviconBytes = mDecodand;
        result.mOffset = mOffset;
        result.mLength = mLen;
        result.mIsICO = true;

        return result;
    }

    


    private class ICOIterator implements Iterator<Bitmap> {
        private int mIndex = 0;

        @Override
        public boolean hasNext() {
            return mIndex < mIconDirectory.length;
        }

        @Override
        public Bitmap next() {
            if (mIndex > mIconDirectory.length) {
                throw new NoSuchElementException("No more elements in this ICO.");
            }
            return decodeBitmapAtIndex(mIndex++);
        }

        @Override
        public void remove() {
            if (mIconDirectory[mIndex] == null) {
                throw new IllegalStateException("Remove already called for element " + mIndex);
            }
            mIconDirectory[mIndex] = null;
        }
    }
}
