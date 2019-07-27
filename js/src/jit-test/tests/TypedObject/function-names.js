
if (typeof TypedObject === "undefined")
    quit();


assertEq(TypedObject.ArrayType.name, "ArrayType");
assertEq(TypedObject.StructType.name, "StructType");
assertEq(TypedObject.storage.name, "storage");
