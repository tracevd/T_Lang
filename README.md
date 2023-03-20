# T_Lang
T language
- A C++ - Like language
- Const by default
  -            T           <>         C++
  -         String x       ==    const String x
  -     mutable String x   ==       String x

- Different Pointer/Reference syntax
  -          T        <>       C++
  -       String~     ==     String&
  -       String->    ==     String*
  -     @<variable>   ==   &<variable>
- Only allows one pointer depth (int-> only) unlike C++, which allows variable pointer depth (int* or int*****)
