

























package ch.boye.httpclientandroidlib.auth;

import java.io.Serializable;
import java.security.Principal;
import java.util.Locale;

import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.LangUtils;






@Immutable
public class NTUserPrincipal implements Principal, Serializable {

    private static final long serialVersionUID = -6870169797924406894L;

    private final String username;
    private final String domain;
    private final String ntname;

    public NTUserPrincipal(
            final String domain,
            final String username) {
        super();
        Args.notNull(username, "User name");
        this.username = username;
        if (domain != null) {
            this.domain = domain.toUpperCase(Locale.ENGLISH);
        } else {
            this.domain = null;
        }
        if (this.domain != null && this.domain.length() > 0) {
            final StringBuilder buffer = new StringBuilder();
            buffer.append(this.domain);
            buffer.append('\\');
            buffer.append(this.username);
            this.ntname = buffer.toString();
        } else {
            this.ntname = this.username;
        }
    }

    public String getName() {
        return this.ntname;
    }

    public String getDomain() {
        return this.domain;
    }

    public String getUsername() {
        return this.username;
    }

    @Override
    public int hashCode() {
        int hash = LangUtils.HASH_SEED;
        hash = LangUtils.hashCode(hash, this.username);
        hash = LangUtils.hashCode(hash, this.domain);
        return hash;
    }

    @Override
    public boolean equals(final Object o) {
        if (this == o) {
            return true;
        }
        if (o instanceof NTUserPrincipal) {
            final NTUserPrincipal that = (NTUserPrincipal) o;
            if (LangUtils.equals(this.username, that.username)
                    && LangUtils.equals(this.domain, that.domain)) {
                return true;
            }
        }
        return false;
    }

    @Override
    public String toString() {
        return this.ntname;
    }

}
