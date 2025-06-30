// RUN: %clang_cc1 -E %s | grep '^    printf( "%%s" , "Hello" );$'

#define P( x, ...) printf( x __VA_OPT__(,) __VA_ARGS__ )
#define PF( x, ...) P( x __VA_OPT__(,) __VA_ARGS__ )

int main()
{
    PF( "%s", "Hello" );
    PF( a, );
    PF( a );
    PF( , );
    PF( );
}
