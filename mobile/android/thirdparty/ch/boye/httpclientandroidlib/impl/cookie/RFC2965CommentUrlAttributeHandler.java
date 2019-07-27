


























package ch.boye.httpclientandroidlib.impl.cookie;

import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.cookie.Cookie;
import ch.boye.httpclientandroidlib.cookie.CookieAttributeHandler;
import ch.boye.httpclientandroidlib.cookie.CookieOrigin;
import ch.boye.httpclientandroidlib.cookie.MalformedCookieException;
import ch.boye.httpclientandroidlib.cookie.SetCookie;
import ch.boye.httpclientandroidlib.cookie.SetCookie2;






@Immutable
public class RFC2965CommentUrlAttributeHandler implements CookieAttributeHandler {

      public RFC2965CommentUrlAttributeHandler() {
          super();
      }

      public void parse(final SetCookie cookie, final String commenturl)
              throws MalformedCookieException {
          if (cookie instanceof SetCookie2) {
              final SetCookie2 cookie2 = (SetCookie2) cookie;
              cookie2.setCommentURL(commenturl);
          }
      }

      public void validate(final Cookie cookie, final CookieOrigin origin)
              throws MalformedCookieException {
      }

      public boolean match(final Cookie cookie, final CookieOrigin origin) {
          return true;
      }

  }
