inline int add(int a, int b) {
    return a + b;
}

void test_vla(int n) {
    int vla[n];
    vla[0] = 1;
}

void test_modern_types(int * restrict p) {
    _Bool b = 1;
    float _Complex c = 1.0f;
}
