#include <iostream>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <boost/process.hpp>

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
    std::string slave_name;
    slave_name = ptsname(fdm);
    std::cout << slave_name << '\n';

    namespace bp = boost::process;
    bp::child c("bash", (bp::std_out & bp::std_err) > slave_name, bp::std_in < slave_name);

    sleep(1);
    char buf[2048];
    read(fdm, buf, 2048);
    std::cout << buf;
    close(fdm);

    return 0;
}
