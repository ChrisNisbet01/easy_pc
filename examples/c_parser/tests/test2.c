typedef int my_int;
typedef my_int * my_int_ptr;

void main(void)
{
    my_int a = 10;
    my_int_ptr b = &a;
    
    a = a + *b;
}
