// Basic function pointer
int (*f)(void);

// Array of function pointers
int (*a[10])(int);

// Function returning a pointer to a function
int (*get_handler(int))(int, int);

// Nested parentheses in declarator
int ((*p));

void main(void) {
    int (*local_f)(int) = 0;
}
