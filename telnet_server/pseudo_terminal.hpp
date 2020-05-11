#ifndef PSEUDO_TERMINAL_HPP
#define PSEUDO_TERMINAL_HPP
#include <unistd.h>
#include <string>

class pty{
public:
    pty();
    pty(pty&& other);
    pty& operator=(pty&& other);
    ~pty();
    pty(const pty& other) = delete;
    pty& operator=(const pty& other) = delete;

    const std::string& pslave_name()
        {return pslave_name_;}
private:
    pid_t pmaster_id_;
    std::string pslave_name_;
};

#endif // PSEUDO_TERMINAL
