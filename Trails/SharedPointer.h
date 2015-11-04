
#include <boost\config.hpp>
#include <boost\shared_ptr.hpp>
#include <iostream>
#include<list>
using boost::shared_ptr;
class IPrintable
{
public:
	virtual void VPrint() = 0;
};

//Inherited IPrintable  Interface
class CPrintable : public IPrintable
{
	char *m_Name;
public:
	//Default CTOR	
	CPrintable(char *name)
	{
		m_Name = name; 
		printf("create	%s\n", m_Name);
	}
	//DTOR
	virtual ~CPrintable()
	{
		printf("delete %s\n", m_Name);
	}
	//Print
	void VPrint()
	{
		printf("print %s\n", m_Name);
	}
};

