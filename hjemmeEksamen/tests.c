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

char *handle_response(char *buf)
{
    char *response, mBuf[strlen(buf) + 1];
    strcpy(mBuf, buf);
    char *pkt = strtok(mBuf, " ");
    printf("pkt: %s\n", pkt);
    char *number = strtok(NULL, " ");
    printf("number: %s\n", number);
    char *operation = strtok(NULL, " ");
    char *nick = strtok(NULL, " ");

    if (!strcmp(operation, "REG"))
    {
        printf("registering %s\n", nick);
        response = "registered";
    }
    else if (!strcmp(operation, "LOOKUP"))
    {
        printf("looking up %s\n", nick);
        response = "looked up";
    }
    else
    {
        response = "WRONG FORMAT";
    }

    return response;
}

void test_handle_response()
{
    char *a = handle_response("PKT 0 REG bob");
    char *b = handle_response("PKT 0 REG alice");
    char *c = handle_response("PKT 1 LOOKUP bob");

    printf("Responses: %s, %s, %s\n", a, b, c);
}

/**
 *   Registration message: "PKT number REG nick"
 *   Registration ok: "ACK number OK"
 *   Lookup message: "PKT number LOOKUP nick"
 *   Lookup fail response: "ACK number NOT FOUND"
 *   Lookup success response: "ACK number NICK nick IP address PORT port"
 *   Text message: "PKT number FROM fromnick TO tonick MSG text"
 *   Text response: "ACK number [OK/WRONG NAME/WRONG FORMAT]"
 *      PKT 0 REG bob
 *      PKT 0 REG alice
 *      PKT 1 LOOKUP bob
 *      PKT 1 LOOKUP bobo
 *      q
        PKT 0 REG bob
        PKT 0 REG alice
        PKT 1 LOOKUP bob
        PKT 1 LOOKUP bobo
        q
 */

int main(void)
{
    // test_convert_loss_probability();
    test_handle_response();
    /* code */
    return 0;
}
