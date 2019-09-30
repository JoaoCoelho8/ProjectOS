#include "my_protocol.h"


int main()
{
    char* js="{\"type\":1,\"origin\":\"127.0.0.1:8080\",\"destination\":\"127.0.0.1:8081\",\"message\":\"olá\"}";

    PROTOCOL* protocol=parse_message(js);
    print_protocol(protocol);

    return 0;
}
