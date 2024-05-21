#include "tests.h"
#include "stdlib.h"

int main() {
    sf_t *sf = (sf_t *)malloc(sizeof(sf_t));
    if (run_tests(sf) < 0) {
        return -1;
    }
    return 0;
}
