









































gTestfile = '11.2.4.js';

START("11.2.4 - XML Filtering Predicate Operator");

e = <employees>
    <employee id="0"><name>John</name><age>20</age></employee>
    <employee id="1"><name>Sue</name><age>30</age></employee>
    </employees>;


correct = <employee id="0"><name>John</name><age>20</age></employee>;

john = e.employee.(name == "John");
TEST(1, correct, john);   

john = e.employee.(name == "John");
TEST(2, correct, john);   

correct =
<employee id="0"><name>John</name><age>20</age></employee> +
<employee id="1"><name>Sue</name><age>30</age></employee>;

twoEmployees = e.employee.(@id == 0 || @id == 1);
TEST(3, correct, twoEmployees);

twoEmployees = e.employee.(@id == 0 || @id == 1);
TEST(4, correct, twoEmployees);

i = 0;
twoEmployees = new XMLList();
for each (var p in e..employee)
{
    if (p.@id == 0 || p.@id == 1)
    {
        twoEmployees += p;
    }
}
TEST(5, correct, twoEmployees);

i = 0;
twoEmployees = new XMLList();
for each (var p in e..employee)
{
    if (p.@id == 0 || p.@id == 1)
    {
        twoEmployees[i++] = p;
    }
}
TEST(6, correct, twoEmployees);


e = <employees>
    <employee id="0"><name>John</name><age>20</age></employee>
    <employee id="1"><name>Sue</name><age>30</age></employee>
    </employees>;

correct =
<employee id="0"><name>John</name><age>20</age></employee> +
<employee id="1"><name>Sue</name><age>30</age></employee>;

i = 0;
twoEmployees = new XMLList();
for each (var p in e..employee)
{
    with (p)
    {
        if (@id == 0 || @id == 1)
        {
            twoEmployees[i++] = p;
        }
    }
}
TEST(7, correct, twoEmployees);

END();
