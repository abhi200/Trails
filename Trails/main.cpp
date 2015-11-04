#include "SharedPointer.h"

shared_ptr<CPrintable> CreateAnObject(char *name)
{
	return shared_ptr<CPrintable>(new CPrintable(name));
}

void ProcessObject(shared_ptr<CPrintable> o)
{
	printf("(print from a function) ");
	o->VPrint();
}

void TestSharedPointers(void)
{
	shared_ptr<CPrintable> ptr1(new CPrintable("1")); // create object 1
	shared_ptr<CPrintable> ptr2(new CPrintable("2")); // create object 2
	ptr1 = ptr2; // destroy object 1
	ptr2 = shared_ptr<CPrintable>(new CPrintable("3")); // used as a return value
	ProcessObject(ptr1); // call a function
	CPrintable o1("bad");
	//ptr1 = &o1; // Syntax error! It's on the stack....
	//
	CPrintable *o2 = new CPrintable("bad2");
	//ptr1 = o2; // Syntax error! Use the next line to do this...
	ptr1 = shared_ptr<CPrintable>(o2);
	// You can even use shared_ptr on ints!
	shared_ptr<int> a(new int);
	shared_ptr<int> b(new int);
	*a = 5;
	*b = 6;
	const int *q = a.get(); // use this for reading in multithreaded code
							// this is especially cool - you can also use it in lists.
	std::list< shared_ptr<int> > intList;
	std::list< shared_ptr<IPrintable> > printableList;
	for (int i = 0; i<100; ++i)
	{
		intList.push_back(shared_ptr<int>(new int(rand())));
		printableList.push_back(shared_ptr<IPrintable>(new CPrintable("list")));
	}
}

void main()
{
	TestSharedPointers();
}

