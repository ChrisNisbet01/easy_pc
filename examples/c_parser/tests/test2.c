/* Structs and complex expressions */
struct Point
{
    int x;
    int y;
};

static struct Point global_pt;

int
test_expr(int a, int b)
{
    int res = (a << 2) | (b >> 1);
    res = (a && b) ? a : b;
    res = sizeof(a) + sizeof global_pt;
    return !res || ~a ^ b;
}

void
main(void)
{
    global_pt.x = 10;
    global_pt.y = 20;
    test_expr(global_pt.x, global_pt.y);
}
