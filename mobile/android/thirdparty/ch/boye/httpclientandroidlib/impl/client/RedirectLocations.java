


























package ch.boye.httpclientandroidlib.impl.client;

import java.net.URI;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;

import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;






@NotThreadSafe 
public class RedirectLocations {

    private final Set<URI> unique;
    private final List<URI> all;

    public RedirectLocations() {
        super();
        this.unique = new HashSet<URI>();
        this.all = new ArrayList<URI>();
    }

    


    public boolean contains(final URI uri) {
        return this.unique.contains(uri);
    }

    


    public void add(final URI uri) {
        this.unique.add(uri);
        this.all.add(uri);
    }

    


    public boolean remove(final URI uri) {
        boolean removed = this.unique.remove(uri);
        if (removed) {
            Iterator<URI> it = this.all.iterator();
            while (it.hasNext()) {
                URI current = it.next();
                if (current.equals(uri)) {
                    it.remove();
                }
            }
        }
        return removed;
    }

    






    public List<URI> getAll() {
        return new ArrayList<URI>(this.all);
    }

}
