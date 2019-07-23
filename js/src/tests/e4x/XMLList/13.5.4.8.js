









































gTestfile = '13.5.4.8.js';

START("13.5.4.8 - XMLList copy()");

TEST(1, true, XMLList.prototype.hasOwnProperty("copy"));

emps = new XMLList();
emps += <employee id="0"><name>Jim</name><age>25</age></employee>;
emps += <employee id="1"><name>Joe</name><age>20</age></employee>;

correct = new XMLList();
correct += <employee id="0"><name>Jim</name><age>25</age></employee>;
correct += <employee id="1"><name>Joe</name><age>20</age></employee>;

TEST(2, emps, emps.copy());
TEST(3, correct, emps.copy());


emps = new XMLList();
emps += <employee id="0"><name>Jim</name><age>25</age></employee>;
emps += <employee id="1"><name>Joe</name><age>20</age></employee>;
  
correct = new XMLList();
correct += <employee id="0"><name>Jim</name><age>25</age></employee>;
correct += <employee id="1"><name>Joe</name><age>20</age></employee>;

x = emps.copy();

emps += <employee id="2"><name>Sue</name><age>32</age></employee>;

TEST(4, correct, x);

END();