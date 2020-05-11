#include "pseudo_terminal.hpp"

#include <string>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>

pty::pty()
{
    int ec;
    pmaster_id_ = posix_openpt(O_RDWR);
    if(pmaster_id_ < 0)
        throw std::runtime_error("Error in creating pty - posix_openpt()");
    ec = grantpt(pmaster_id_);
    if(ec)
        throw std::runtime_error("Error in creating pty - grant_pt()");
    ec = unlockpt(pmaster_id_);
    if(ec)
        throw std::runtime_error("Error in creating pty - unlockpt()");
    try {
        pslave_name_ = ptsname(pmaster_id_); //ptsname can return NULL if slave terminal can not be created
    }  catch (const std::exception& e) {
        std::cerr << "Error in ptsname " << e.what() << '\n';
    }
}

pty::pty(pty&& other)
    : pmaster_id_(std::exchange(other.pmaster_id_, -1)),
    pslave_name_(std::move(other.pslave_name_))
{}

pty& pty::operator=(pty &&other)
{
    pmaster_id_ = std::exchange(other.pmaster_id_, -1);
    pslave_name_ = std::move(other.pslave_name_);
    return *this;
}

pty::~pty()
{
    close(pmaster_id_);
}
