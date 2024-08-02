#include <bits/stdc++.h>
using namespace std;


struct A
{
	int x;
	int y;

	A() : x{1}, y{2} {}
	virtual ~A() = default;

	void foo()
	{
		cout << x << ' ' << y << '\n';
		cout << sizeof(A) << '\n';
	}
};


struct B : public A
{
	int x;
	int y;

	B() : x{3}, y{4} {}

	inline void foo()
	{
		cout << A::x << ' ' << A::y << '\n';
		cout << B::x << ' ' << B::y << '\n';
	}
};

int main(int argc, const char* argv[], const char* envp[])
{
	A* a = new B;
	a->foo();
	delete a;
}

