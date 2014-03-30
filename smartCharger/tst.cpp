#include <string>
#include <cstdarg>
#include <iostream>
#include <dirent.h>
#include <unistd.h>
#include <signal.h>

#define BINARYLOCATION_TAR "/usr/bin/tar"

int main(int argc, char const *argv[])
{
  // char *args[] = {"/bin/ls", "-ls", NULL};
  char* args[3];
  args[0] = "/bin/ls";
  args[1] = "-ls";
  args[2] = NULL;
  execv("/bin/ls", args);
}

