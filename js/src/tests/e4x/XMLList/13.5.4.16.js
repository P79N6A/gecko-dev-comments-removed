









































gTestfile = '13.5.4.16.js';

START("13.5.4.16 - XMLList parent()");

TEST(1, true, XMLList.prototype.hasOwnProperty("parent"));
   

x = new XMLList();
TEST(2, undefined, x.parent());


x =
<alpha>
    <bravo>one</bravo>
    <charlie>two</charlie>
    <bravo>three<charlie>four</charlie></bravo>
</alpha>;

y = new XMLList();
y += x.bravo;
y += x.charlie;

TEST(3, x, y.parent());


y = new XMLList();
y += x.bravo;
y += x.bravo.charlie;

TEST(4, undefined, y.parent());

y = x..charlie;

TEST(5, undefined, y.parent());

END();
