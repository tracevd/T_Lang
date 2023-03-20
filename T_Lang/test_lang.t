class Human
{
public:
	Human constructor() {}
	int8 getAge() { return age; }
	mutable String~ getName() { return name; }
	void setName( String~ name_ ) { name = name_; }
private:
	String name;
	int8 age;
}

//String concat( String~ lhs, mutable String~ rhs )
//{
//	rhs = "h";
//	return lhs + rhs == " hello";
//}

uint8 x = y = 5;

mutable Human h;
h.getName( "bob" ).substr( 0, -2.056 );
bool b = true;
char c = 'h';

if ( c.getThing() == 'h' )
{
	c = 'g';
}

auto g = c . getThing() / ( 4 == 7.35 ) ** 45;

namespace test
{
	class Human
	{
	private:
		String-> name;
	}
}
