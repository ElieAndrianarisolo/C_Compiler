int toto2(){
	return a;
}

int toto() {
	int a = 1+1;
	return toto2();
}

int main() {
    int a = toto();
    return a;
}
