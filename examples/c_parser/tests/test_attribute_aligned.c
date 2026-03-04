struct AlignedStruct
{
    int data1;
    long data2;
} __attribute__((__aligned__(16)));

void
main()
{
    struct AlignedStruct instance;
    // Just to make it a valid C file that compiles/parses
    instance.data1 = 10;
}
