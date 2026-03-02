enum Color
{
    RED,
    GREEN = 10,
    BLUE
};

struct Flags
{
    unsigned int is_active : 1;
    unsigned int priority : 3;
};

void
main(void)
{
    enum Color c = GREEN;

    int arr[] = {
        1,
        2,
        3,
    };
    int matrix[2][2] = {{1, 0}, {0, 1}};

    struct Point
    {
        int x, y;
    } p = {10, 20};

    struct Flags f;
    f.is_active = 1;
}
