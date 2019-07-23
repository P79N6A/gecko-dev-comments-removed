









































gTestfile = '13.4.4.27.js';

START("13.4.4.27 - XML parent()");

TEST(1, true, XML.prototype.hasOwnProperty("parent"));
   
x =
<alpha>
    <bravo>one</bravo>
    <charlie>
        <bravo>two</bravo>
    </charlie>
</alpha>;

y = x.bravo;

TEST(2, x, y.parent());

TEST(3, null, x.parent());

END();
