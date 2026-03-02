void
main(void)
{
    int a, b;
    a = (b = 5, b + 10);

    int c = 0;
    c = a, b; /* This parses as (c = a), b; in C */
}
