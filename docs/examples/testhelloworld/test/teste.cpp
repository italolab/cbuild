#include <test/cbtest.h>

TEST_CASE( primeiroTeste, Testes ) {
    int a = 1;
    int b = 1;
    int c = 2;
    ASSERT_EQUALS( a, b, )
    ASSERT_TRUE( a == b, )
    ASSERT_FALSE( a == c, )
}

int main() {
    RUN_TEST_CASES_MENU();
    return 0;
}