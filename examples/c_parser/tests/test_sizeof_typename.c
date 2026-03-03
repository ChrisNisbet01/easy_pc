struct Point { int x, y; };

void main(void) {
    int a = sizeof(int);
    int b = sizeof(long long);
    int c = sizeof(struct Point);
    int d = sizeof(a);
    int e = sizeof a;
    int f = sizeof(int *);
}
