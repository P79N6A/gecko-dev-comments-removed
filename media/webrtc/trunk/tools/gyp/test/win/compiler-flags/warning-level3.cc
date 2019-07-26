




struct __declspec(align(8)) C8 { __int64 i; };
struct __declspec(align(4)) C4 { C8 m8; };

int main() {
  return 0;
}
