// C99 advanced array sizing in function parameters
void f1(int a[static 10]);
void f2(int a[const 10]);
void f3(int a[static const 10]);
void f4(int a[const static 10]);
void f5(int a[*]);
void f6(int a[restrict static 5]);

void main(void) {}
