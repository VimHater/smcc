int main()
{
    int n = 5;
    int sum = 0;
    while (n > 0) {
        sum = sum + n;
        n = n - 1;
    }
    print_int(sum);
    return 0;
}
