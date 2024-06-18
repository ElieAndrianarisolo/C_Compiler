int main() 
{
    int a = 3;
    int b = 5;
    int d = 7;
    int e = 9;
    int f = 11;
    int g = 13;
    int c = (a ^ b) | f*(d & e) + g;
    return c;
}