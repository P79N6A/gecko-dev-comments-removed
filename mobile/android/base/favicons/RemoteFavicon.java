



package org.mozilla.gecko.favicons;






public class RemoteFavicon implements Comparable<RemoteFavicon> {
    public static final int FAVICON_SIZE_ANY = -1;

    
    public String faviconUrl;

    
    public int declaredSize;

    
    public String mimeType;

    public RemoteFavicon(String faviconURL, int givenSize, String mime) {
        faviconUrl = faviconURL;
        declaredSize = givenSize;
        mimeType = mime;
    }

    





    @Override
    public boolean equals(Object o) {
        if (!(o instanceof RemoteFavicon)) {
            return false;
        }
        RemoteFavicon oCast = (RemoteFavicon) o;

        return oCast.faviconUrl.equals(faviconUrl) &&
               oCast.declaredSize == declaredSize &&
               oCast.mimeType.equals(mimeType);
    }

    @Override
    public int hashCode() {
        return super.hashCode();
    }

    





    @Override
    public int compareTo(RemoteFavicon obj) {
        
        if (declaredSize == FAVICON_SIZE_ANY) {
            if (obj.declaredSize == FAVICON_SIZE_ANY) {
                return 0;
            }

            return -1;
        }

        if (obj.declaredSize == FAVICON_SIZE_ANY) {
            return 1;
        }

        
        if (declaredSize > obj.declaredSize) {
            return -1;
        }

        if (declaredSize == obj.declaredSize) {
            
            
            if (Favicons.isContainerType(mimeType)) {
                return -1;
            }
            return 0;
        }
        return 1;
    }
}
