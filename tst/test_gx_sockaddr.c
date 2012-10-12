#include <assert.h>
#include "../gx.h"
#include "../gx_sockaddr.h"

void test_ipv4()
{
  gx_sockaddr sa;
  char buffer[INET6_ADDRSTRLEN + 1] = { 0 };

  _ (gx_sockaddr_set(&sa, "127.0.0.1", 42))         _abort();
  _ (gx_sockaddr_get(&sa, buffer, sizeof(buffer)))  _abort();
  _E (strcmp(buffer, "127.0.0.1:42"))               _abort();
  _Z ((sizeof(sa.sin4) == gx_sockaddr_length(&sa))) _abort();
}

void test_ipv6()
{
  gx_sockaddr sa;
  char buffer[INET6_ADDRSTRLEN + 1] = { 0 };

  _ (gx_sockaddr_set(&sa, "fe80::20c:29ff:fe31:c03e", 42)) _abort();
  _ (gx_sockaddr_get(&sa, buffer, sizeof(buffer)))         _abort();
  _E (strcmp(buffer, "fe80::20c:29ff:fe31:c03e:42"))       _abort();
  _Z ((sizeof(sa.sin6) == gx_sockaddr_length(&sa)))        _abort();
}

int main()
{
  test_ipv4();
  test_ipv6();
  return 0;
}
