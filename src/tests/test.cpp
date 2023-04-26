#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <time.h>
#include <thread>

bool check_ll2(long long a) {
  volatile double b = (double) a;
  const double d_longLong_max_plus_1 = (LLONG_MAX/2 + 1)*2.0;
  #if LLONG_MIN == -LLONG_MAX
    const double d_longLong_min_minus_1 = (LLONG_MIN/2 - 1)*2.0;;
    if (b <= d_longLong_min_minus_1 || b >= d_longLong_max_plus_1) {
      return false;
    }
  #else
    if (b >= d_longLong_max_plus_1) {
      return false;
    }
  #endif
  return (long long) b == a;
}

#include <WS2tcpip.h>
#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>
#include <string>

int main(int argc, const char** argv) {

	struct WSAData wsa_data;
	SOCKET tcp_socket;
	struct sockaddr_in server_addr;

	if (WSAStartup(MAKEWORD(2,2), &wsa_data) != 0)
		return false;

  IP_ADAPTER_INFO * FixedInfo;
  ULONG ulOutBufLen;

  FixedInfo = (IP_ADAPTER_INFO *) malloc(sizeof( IP_ADAPTER_INFO ) );
  ulOutBufLen = sizeof( IP_ADAPTER_INFO );

  if ( ERROR_SUCCESS != GetAdaptersInfo( FixedInfo, &ulOutBufLen ) )
  {
    printf("Failed to get adapter info...\n");
    return FALSE;
  }

  unsigned long targetMask = inet_addr( FixedInfo->IpAddressList.IpMask.String );
  unsigned long ipMask = inet_addr( FixedInfo->IpAddressList.IpAddress.String );

  printf("IP: %s\nMask: %s\n", 
    FixedInfo->IpAddressList.IpAddress.String, 
    FixedInfo->IpAddressList.IpMask.String);

  int host_bits = 32;
  for (int i = 0; i < 32; i++) {
    if (((targetMask >> i) & 1) == 1)
      --host_bits;
  }

  unsigned max = (unsigned)(pow(2.0, (float)host_bits));

  printf("Max combinations: %u\n", max);

	tcp_socket = socket(AF_INET, SOCK_STREAM, 0);

	if (tcp_socket < 0) 
		return 1;

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(8345);

  auto try_connect = [] (unsigned i, SOCKET sock, unsigned ipMask) {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
	  addr.sin_port = htons(8345);
  
    for (unsigned i = 0; i < 8; i++)
      ipMask &= ~(1 << (i + 24));
    ipMask |= (i << 24);

     addr.sin_addr.s_addr = ipMask;

    printf("C to '%s'\n", inet_ntoa(addr.sin_addr));
  };

  std::thread ths[256];

  for (unsigned i = 0; i < max; i++)
  {
    ths[i] = std::thread(try_connect, i, tcp_socket, ipMask);
    ths[i].detach();
  }
  printf("Opened all threads\n");

  getchar();

  // for (unsigned i = 0; i < max; i++) {
  //   for (unsigned i = 0; i < host_bits; i++)
  //     ipMask &= ~(1 << (i + 24));
  //   ipMask |= (i << 24);


  //   // Try connect
	//   server_addr.sin_addr.s_addr = ipMask;
  //   if (connect(tcp_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
  //     printf("No server at: %s\n", inet_ntoa(server_addr.sin_addr));
  //   }
  //   else {
  //     clock_t end = clock();
  //     double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
  //     printf("Got server: %s after %lf\n", inet_ntoa(server_addr.sin_addr), time_spent);
  //     closesocket(tcp_socket);
  //     exit(1);
  //   }
  // }
}