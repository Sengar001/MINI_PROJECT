#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<fcntl.h>
#include<unistd.h>
#include"./admin_struct.h"
// Define a structure to hold employee data

int main() {
    struct Admin admin = {"admin-0","34Hr82oIlAE8I"};

    const char *filename = "./admin.txt";
    int fd = open(filename, O_RDWR);
    write(fd, &admin, sizeof(admin));

    printf("Employee data written to %s\n", filename);
    return 0;
}