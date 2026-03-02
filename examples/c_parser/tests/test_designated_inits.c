struct Point { int x, y; };

void main(void) {
    // Struct designated initializers
    struct Point p = { .y = 20, .x = 10 };
    
    // Array designated initializers
    int arr[10] = { [0] = 1, [5] = 10, [9] = 100 };
    
    // Nested/Mixed
    struct Point pts[2] = {
        [0] = { .x = 1, .y = 2 },
        [1].x = 3,
        [1].y = 4
    };

    // Trailing comma with designation
    int colors[] = { [0] = 255, [1] = 128, };
}
