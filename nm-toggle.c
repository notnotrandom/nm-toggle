#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NM_START "systemctl start NetworkManager.service"
#define NM_STOP  "systemctl stop  NetworkManager.service"
#define NM_MASK1 "systemctl mask NetworkManager-dispatcher.service"
#define NM_MASK2 "systemctl mask NetworkManager.service"
#define NM_UNMASK1 "systemctl unmask NetworkManager-dispatcher.service"
#define NM_UNMASK2 "systemctl unmask  NetworkManager.service"
#define NM_DISABLE_NETWORKING "nmcli networking off"
#define NM_ENABLE_NETWORKING  "nmcli networking on"
#define NM_DISABLE_WIRELESS "nmcli radio wifi off"

int main(int argc, const char *argv[])
{
  int s;
  char* nm_msg = "NetworkManager UP\n";

  if (argc != 2) {
    printf("Exactly ONE argument is required: on|off\n(any other value is assumed to be off)\n");
    exit(-1);
  }
  if (strncmp(argv[1], "on", 2) == 0) {
    system(NM_UNMASK1);
    system(NM_UNMASK2);

    s = system(NM_START);
    if (WEXITSTATUS(s)==0)
      printf(nm_msg);

    system(NM_ENABLE_NETWORKING);
    system(NM_DISABLE_WIRELESS); // just to be sure...
  } else { // go offline
    nm_msg = "NetworkManager DOWN\n";

    system(NM_DISABLE_WIRELESS); // just to be sure...
    system(NM_DISABLE_NETWORKING);

    s = system(NM_STOP);

    if (WEXITSTATUS(s)==0) {
      system(NM_MASK1);
      s = system(NM_MASK2);

      if (WEXITSTATUS(s)==0)
        printf(nm_msg);
    }
  }

  return 0;
}
