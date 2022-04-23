#include "preCode/send_packet.h"
#include "common.h"
#include <assert.h>
#include <stdio.h>

void test_convert_loss_probability()
{
    int a = 10;
    int b = 0;
    int c = 100;
    int d = 33;

    float ar = convert_loss_probability(10);
    float br = convert_loss_probability(0);
    float cr = convert_loss_probability(100);
    float dr = convert_loss_probability(33);

    assert(ar >= 0.1 && ar < 1.11);
    assert(br == 0.0 && br == 0);
    assert(cr == 1.0 && cr == 1);
    assert(dr >= 0.33 && dr < 0.34);

    printf("Conversion test passed, results: %d-%f, %d-%f, %d-%f, %d-%f\n",
           a, ar, b, br, c, cr, d, dr);
}

int main(void)
{
    test_convert_loss_probability();
    /* code */
    return 0;
}
