void main(void) {
    int T = 5;
    int ptr = 10;
    
    /* In this case, T is NOT a typedef, so (T)*ptr is multiplication */
    int z = (T)*ptr;
}
