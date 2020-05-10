#include <iostream>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

int main()
{
    int fdm, fds, ec;
    fdm = posix_openpt(O_RDWR);
    if(fdm < 0)
        throw std::runtime_error("Error on posix_openpt");
    ec = grantpt(fdm);
    if(ec)
        throw std::runtime_error("Error on grantpt");
    ec = unlockpt(fdm);
    if(ec)
        throw std::runtime_error("Error on unlockpt");
    std::string_view slave_name;
    slave_name = ptsname(fdm);
    std::cout << slave_name << '\n';
    fds = open(slave_name.data(), O_RDWR);

    pid_t childid = fork();
    if(childid == 0){
        close(0);
        close(1);
        close(2);
        dup(fds);
        dup(fds);
        dup(fds);
        system("bash");
    } else {
        int ec;
        pid_t c;
        sleep(1);
        char buf[2048];
        read(fdm, buf, 2048);
        std::cout << buf;
    }


    return 0;
}
