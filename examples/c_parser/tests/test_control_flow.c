void main(void) {
    int i;
    
    /* For loop */
    for (i = 0; i < 10; i++) {
        if (i == 5) continue;
        if (i == 8) break;
    }
    
    /* For loop with declaration (C99-ish, but our grammar supports it) */
    for (int j = 0; j < 5; j++) {
        int x = j;
    }
    
    /* Do-while loop */
    int k = 0;
    do {
        k++;
    } while (k < 5);
    
    /* Switch statement */
    switch (k) {
        case 1:
            k = 10;
            break;
        case 2:
        case 3:
            k = 20;
            break;
        default:
            k = 0;
            break;
    }
    
    /* Goto and Label */
    if (k == 0) goto end;
    k = 1;
    
end:
    return;
}
