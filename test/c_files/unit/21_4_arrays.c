/*d array with ptr arithm. */
/*@ 3 0 0 3 */
/*@ 5 23 0 28 */
/*@ 6 -10 0 -4 */
/*@ 0 0 0 0 */

int func(int a, int b, int c) {
    int arr[12];
    *(arr + 5) = a;
    arr[6] = b;
    *(arr + 7) = *(arr + 5) + arr[6];
    return arr[7];
}

