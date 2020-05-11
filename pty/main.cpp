#include <iostream>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include <boost/process.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>

int main()
{
    try {
        int fdm, ec;
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



        boost::asio::io_context ctx;
        boost::asio::signal_set stop_signals(ctx, SIGTERM, SIGINT);
        stop_signals.async_wait([&](boost::system::error_code ec, int /*signal*/){
            if(ec)
                return;
            std::cout << "Terminating in response to signal";
            ctx.stop();
        });

        boost::asio::spawn(ctx, [&ctx, fdm, slave_name=std::move(slave_name)](boost::asio::yield_context yc){
            std::shared_ptr<boost::asio::posix::stream_descriptor> master_m =
                std::make_shared<boost::asio::posix::stream_descriptor>(boost::asio::posix::stream_descriptor(ctx, fdm));
            std::mutex m;

            namespace bp = boost::process;
            bp::child c(ctx, "bash -i -s", (bp::std_out & bp::std_err) > slave_name, bp::std_in < slave_name);

            std::cout << "Is open: " << master_m->is_open() << '\n';

            boost::asio::spawn(ctx, [master_m, &m](boost::asio::yield_context yc){
                boost::system::error_code ec;
                std::string out_buf;
                out_buf.resize(1024);
                for(;;){
                    std::size_t str_size = master_m->async_read_some(boost::asio::buffer(out_buf, out_buf.size()), yc[ec]);
                    out_buf.resize(str_size);
                    std::cerr << out_buf << '\n';
                }
            });

            boost::system::error_code ec;
            std::string in_buf;
            for(;;){
                in_buf.resize(0);
                std::cin >> in_buf;
                in_buf.push_back('\n');
                boost::asio::async_write(*master_m, boost::asio::buffer(in_buf, in_buf.size()), yc[ec]);
            }
        });

        ctx.run();

    }  catch (const std::exception& e) {
        std::cerr << e.what();
    }

    return 0;
}
