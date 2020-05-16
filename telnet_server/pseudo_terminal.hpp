#ifndef PSEUDO_TERMINAL_HPP
#define PSEUDO_TERMINAL_HPP
#include <unistd.h>
#include <string>
#include <boost/asio/io_context.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>

class pty{
public:
    pty(boost::asio::io_context& ctx);
    pty(pty&& other);
    pty& operator=(pty&& other);
    ~pty();
    pty(const pty& other) = delete;
    pty& operator=(const pty& other) = delete;
    const std::string& pslave_name()
        {return pslave_name_;}

    boost::asio::posix::stream_descriptor master;
private:
    pid_t pmaster_id_;
    std::string pslave_name_;
};

#endif // PSEUDO_TERMINAL
