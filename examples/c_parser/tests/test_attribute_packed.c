struct PackedStruct {
    char a;
    int b;
} __attribute__((packed));

void main() {
    struct PackedStruct instance;
    // Just to make it a valid C file that compiles/parses
    instance.a = 'x';
}
