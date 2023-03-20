# T_Lang
T language
- A C++ - Like language
- Const by default
  -            T                 C++ Equivalent
  -         String x       ==    const String x
  -     mutable String x   ==       String x

- Different Pointer/Reference syntax
  -          T             C++ Equivalent
  -       String~      ==      String&
  -       String->     ==      String*
  -     @<variable>    ==    &<variable>
- Only allows one pointer depth (int-> only) unlike C++, which allows variable pointer depth (int* or int*****)
