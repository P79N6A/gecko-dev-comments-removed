


























package ch.boye.httpclientandroidlib.pool;

import java.io.IOException;









public interface ConnFactory<T, C> {

    C create(T route) throws IOException;

}
