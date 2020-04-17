#define _GNU_SOURCE

// NOTA BENE: change "#undef" to "#define" for debug output.
#undef DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// for nanosleep
#include <time.h>

// for fork & wait
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

// for open()
#include <sys/stat.h>
#include <fcntl.h>
/* #include <sys/types.h> --> already included for fork() */

#define PATH "/usr/bin/"
#define MAX_NUM_TOKENS 8

// Commands to take the machine offline, in the order they are to be executed.
#define NM_DISABLE_WIRELESS   "nmcli radio wifi off"
#define NM_DISABLE_NETWORKING "nmcli networking off"
#define NM_STOP    "systemctl stop NetworkManager.service"
#define NM_MASK1   "systemctl mask NetworkManager-dispatcher.service"
#define NM_MASK2   "systemctl mask NetworkManager.service"

// Commands to take the machine online, in the order they are to be executed.
// (But note that wireless is still kept disabled; it has to be brought on manually.)
#define NM_UNMASK1 "systemctl unmask NetworkManager-dispatcher.service"
#define NM_UNMASK2 "systemctl unmask NetworkManager.service"
#define NM_START   "systemctl start  NetworkManager.service"
#define NM_ENABLE_NETWORKING "nmcli networking on"
#define NM_ENABLE_WIRELESS   "nmcli radio wifi on"

// Check if NetworkManager is running.
// Exits with status 0 if is running, non-zero otherwise.
#define NM_CHECK_IS_RUNNING "systemctl is-active --quiet NetworkManager"

#define PING_GOOGLE "ping -c 2 www.google.com"

static char* env[] = {"PATH="PATH"\0", (char*) 0};

// Struct for nanosleep:
// sleep_length.tv_sec = 0; // seconds
// sleep_length.tv_nsec = 10 * 1000000; // 10 miliseconds
static struct timespec sleep_length = { 0, 10 * 1000000 };

// E.g. systemctl -> /sbin/systemctl
void get_full_path(const char* executable_name, char* path)
{
  if (strcmp(executable_name, "nmcli") ==0)
    strcpy(path, "/usr/bin/nmcli");
  else if (strcmp(executable_name, "ping") == 0)
    strcpy(path, "/usr/bin/ping");
  else if (strcmp(executable_name, "systemctl") == 0)
    strcpy(path, "/sbin/systemctl");
  else
    path = NULL;
}

// Print & flush!
void pf(const char* s)
{
  printf("%s", s);
  fflush(stdout);
}

/*
 * @brief: Fork, Exec, and Wait...
 *
 * @return the exit status of the child.
 */
int few(const char* command, bool quiet)
{
  int i = 0;                      // Counter for for-loop.
  int ret = 0;                    // To keep return value of execve() -- needed when the call b0rks.
  int status;                     // Needed to see if child succeeded.
  pid_t c_pid;                    // To store pid of child.
  char* command_local_cpy = NULL; // To get a non-const of "command" argument (needed for strtok()).
  char* arguments[MAX_NUM_TOKENS] = \
  {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}; // To store the arguments of the command to be executed by exec().
  char command_path[128];         // Place to store the full path of the executable to be run (needed by exec()).

  if (strlen(command) == 0) { // Balk if empty command given.
    fprintf(stderr, "Empty command given?");
    fflush(stdout);
    return -1;
  }

  if (!quiet) {
    pf(command); // Tell the user...
    pf("... "); // ... what we are about to do.
  }

  c_pid = fork();

  if( c_pid == 0 ){ // child
#ifndef DEBUG
    // File descriptor used to map fork'd child's output and err to
    // /dev/null. It is defined here because otherwise, when not in
    // debug mode, compiler complains about unused variable...
    int fd;

    /* open /dev/null for writing */
    fd = open("/dev/null", O_WRONLY);
    dup2(fd, 1);    /* make stdout a copy of fd (> /dev/null) */
    dup2(fd, 2);    /* ...and same with stderr */
    close(fd);
#endif

    command_local_cpy = strdup(command); // Make modifiable copy.
    arguments[0] = strtok(command_local_cpy, " "); // Tokenize command string.
    for (i = 1; i < MAX_NUM_TOKENS; i++) {
      arguments[i] = strtok(NULL, " ");
      if (arguments[i] == NULL) break;
    }
    if (i < MAX_NUM_TOKENS) arguments[i] = (char*) 0; // The last element of the arguments array...
    else arguments[8] = (char*) 0; //... must be (char*) 0.

    // Balk if no full path for executable can be found.
    get_full_path(arguments[0], command_path);
    if (command_path == NULL) {
      fprintf(stderr, "Can't find full path for command \"%s\"!\n", arguments[0]);
      return(-1);
    }

    ret = execve(command_path, arguments, env); // Finally execute the command!
    if (ret == -1) { // Opps!
      fprintf(stderr, "%s\n", strerror(errno));
      _exit(-1);
    }
  }else if (c_pid > 0){ // Parent, so must wait for child...
    c_pid = wait(&status);
    if ( WIFEXITED(status) ) { // Child exited...
      if (!quiet) {
        if (WEXITSTATUS(status) == 0)
          pf("Done!\n");
        else
          pf("B0rK!!\n");
      }
      return WEXITSTATUS(status);
    }
  }else{
    perror("fork failed");
    return(-1);
  }
  return(-1);
}

// Return 1 if offline, 0 if online.
bool check_machine_is_offline()
{
  printf("checking offline...\n");
  if (few(PING_GOOGLE, true) == 0) // If we can ping google, machine is ONLINE!
    return false;
  else
    return true; // else, we are OFFLINE.
}

void go_online()
{
  char nm_msg[] = "\nNetworkManager is UP and wired networks are enabled!\n";

  few(NM_UNMASK1, false);
  few(NM_UNMASK2, false);

  few(NM_START, false);
  while (few(NM_CHECK_IS_RUNNING, true) != 0) { // While NetworkManager is NOT running...
    printf("NetworkManager still not up, trying again after 10 millisec...");
    nanosleep(&sleep_length, NULL);

    // Note that if when control reaches here, NetworkManager has already started, re-issuing the start command again does no harm.
    few(NM_START, false);
  }

  // With NetworkManager up and running, bring up wired and wireless interfaces.
  few(NM_ENABLE_NETWORKING, false);
  few(NM_ENABLE_WIRELESS, false);

  pf(nm_msg);
}

void go_offline()
{
  char nm_msg[] = "\nMachine is OFFLINE!\n";

  if (few(NM_CHECK_IS_RUNNING, true) == 0) { // NetworkManager is running, so start by disabling interfaces...
    few(NM_DISABLE_WIRELESS, false);
    few(NM_DISABLE_NETWORKING, false);
    nanosleep(&sleep_length, NULL);

    // ... and then disable NetworkManager.
    few(NM_STOP, false);
    while (few(NM_CHECK_IS_RUNNING, true) == 0) { // If NetworkManager is still running...
      printf("NetworkManager still up, trying again to shutdown after 1 sec...");
      nanosleep(&sleep_length, NULL);

      // Note that if when control reaches here, NetworkManager has already stopped, re-issuing the stop command again does no harm.
      few(NM_STOP, false);
    }

    // With NetworkManager disabled, all is left is to mask it.
    few(NM_MASK1, false);
    few(NM_MASK2, false);
  } else { // NetworkManager NOT running, so check if machine is offline.
    // If machine is offline, and NetworkManager is disabled, there is nothing
    // else to do. Else... if machine is ONLINE, but NetworkManager is DISABLED,
    // then bring everything up properly, and then shut everything down, properly.
    if (check_machine_is_offline() == false) { // Machine is online!
      do {
        go_online();
        nanosleep(&sleep_length, NULL);

        few(NM_DISABLE_WIRELESS, false);
        few(NM_DISABLE_NETWORKING, false);
        nanosleep(&sleep_length, NULL);

        few(NM_STOP, false);
        while (few(NM_CHECK_IS_RUNNING, true) == 0) { // If NetworkManager is STILL running...
          printf("NetworkManager STILL up, trying again to shutdown after 1 sec...");
          nanosleep(&sleep_length, NULL);

          // Note that if when control reaches here, NetworkManager has already stopped, re-issuing the stop command again does no harm.
          few(NM_STOP, false);
        }

        // Again, with NetworkManager disabled, all is left is to mask it.
        few(NM_MASK1, false);
        few(NM_MASK2, false);
      } while(check_machine_is_offline() == false);
    }
  }

  pf(nm_msg);
}

int main(int argc, const char *argv[])
{
  if (argc != 2) {
    printf("Exactly ONE argument is required: on|off\n(any other value is assumed to be off)\n");
    exit(-1);
  }
  if (strncmp(argv[1], "on", 2) == 0) { // going online
    go_online();
  } else { // go offline
    go_offline();
  }
  return 0;
}
