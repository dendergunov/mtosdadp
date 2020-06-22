#include "pseudo_terminal.hpp"

#include <string>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <boost/asio/io_context.hpp>

pty::pty(boost::asio::io_context& ctx)
    :   master(ctx),
    pmaster_id_(posix_openpt(O_RDWR | O_NOCTTY))
{
    int ec;
    if(pmaster_id_ < 0)
        throw std::runtime_error("Error in creating pty - posix_openpt()");
    ec = grantpt(pmaster_id_);
    if(ec){
        close(pmaster_id_);
        throw std::runtime_error("Error in creating pty - grant_pt()");
    }

    ec = unlockpt(pmaster_id_);
    if(ec){
        close(pmaster_id_);
        throw std::runtime_error("Error in creating pty - unlockpt()");
    }

    pslave_name_.resize(60);
    ec = ptsname_r(pmaster_id_, pslave_name_.data(), pslave_name_.size());
    if(ec){
        close(pmaster_id_);
        throw std::runtime_error("Error in ptsname_r\n");
    }

    master.assign(pmaster_id_);
}

pty::pty(pty&& other)
    :   master(std::move(other.master)),
    pmaster_id_(std::exchange(other.pmaster_id_, -1)),
    pslave_name_(std::move(other.pslave_name_))
{}

pty& pty::operator=(pty &&other)
{
    pmaster_id_ = std::exchange(other.pmaster_id_, -1);
    pslave_name_ = std::move(other.pslave_name_);
    master = std::move(other.master);
    return *this;
}

pty::~pty()
{
    //as posix::stream_descriptor owns the native descriptor, it has to close master pty in its destructor call
}
