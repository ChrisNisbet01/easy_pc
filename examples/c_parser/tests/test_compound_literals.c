struct Point { int x, y; };

void print_point(struct Point p) {}

void main(void) {
    // Basic compound literal for struct
    struct Point p = (struct Point){ .x = 10, .y = 20 };
    
    // Array compound literal
    int * ptr = (int[]){ 1, 2, 3 };
    
    // Compound literal as function argument
    print_point((struct Point){ 5, 5 });
    
    // Nested compound literals
    struct Point * pair = (struct Point[]){ {1, 2}, {3, 4} };
}
