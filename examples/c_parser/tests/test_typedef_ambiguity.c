typedef int T;

void
main(void)
{
    int x = 5;

    /* This should be parsed as a cast of 'x' to type 'T' (int) */
    int y = (T)x;

    /* This should be parsed as a cast of '*ptr' to type 'T' */
    int * ptr = &x;
    int z = (T)*ptr;

    /* Note: In C, (T)*x is a cast. If T was a variable, it would be (T) * x (multiplication).
       Our parser needs typedef support to know T is a type. */
}
