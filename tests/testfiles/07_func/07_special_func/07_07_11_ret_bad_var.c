int main() {
    int a = toto();
    return a;
}

int toto2(){
	return a;
}

int toto() {
	int a = 1+1;
	return toto2();
}