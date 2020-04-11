//  Copyright 2020 Sophos Limited. All rights reserved.

#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>
#include <netdb.h>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>

class Address
{
    struct sockaddr_in addr_{};

public:
    Address(std::string_view hostname, unsigned short port = 80U)
    {
        struct hostent* server{};

        server = ::gethostbyname(hostname.data());
        if (server == NULL)
        {
            throw std::system_error(errno, std::system_category(), "Failed to get IP address of remote host");
        }
        addr_.sin_family = AF_INET;
        ::memcpy(&addr_.sin_addr.s_addr, server->h_addr, server->h_length);
        addr_.sin_port = htons(port);
    }

    operator struct sockaddr*() { return reinterpret_cast<struct sockaddr*>(&addr_); }
    size_t size() const { return sizeof(addr_); }
};

class Connection
{
    int sock_{-1};

public:
    ~Connection()
    {
        if (sock_ != -1)
        {
            ::close(sock_);
        }
    }

    Connection()
    {
        sock_ = ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock_ == -1)
        {
            throw std::system_error(errno, std::system_category(), "Failed to create socket");
        }
    }

    void Connect(std::string_view address)
    {
        Address addr(address, 80);

        if (::connect(sock_, addr, addr.size()) == -1)
        {
            throw std::system_error(errno, std::system_category(), "Failed to connect");
        }
    }

    std::string GetPeerName() const
    {
        struct sockaddr_storage peeraddr{};
        socklen_t len = sizeof(peeraddr);

        if (::getpeername(sock_, reinterpret_cast<struct sockaddr*>(&peeraddr), &len) == -1)
        {
            throw std::system_error(errno, std::system_category(), "Failed to get peer address");
        }

        const char* c = ::inet_ntoa(reinterpret_cast<sockaddr_in*>(&peeraddr)->sin_addr);
        return std::string{c};
    }

    void Send(std::string_view request)
    {
        if (write(sock_, request.data(), request.size()) != static_cast<ssize_t>(request.size()))
        {
            throw std::system_error(errno, std::system_category(), "Failed to write request");
        }
    }

    bool CanRead() const
    {
        fd_set read_fds{};

        FD_ZERO(&read_fds);
        FD_SET(sock_, &read_fds);
        struct timeval tm = {1, 0};
        int rc = ::select(sock_ + 1, &read_fds, nullptr, nullptr, &tm);
        if (rc == -1)
        {
            throw std::system_error(errno, std::system_category(), "failure in select()");
        }
        else if (rc == 0)
        {
            return false;
        }
        else if (!FD_ISSET(sock_, &read_fds))
        {
            throw std::system_error(errno, std::system_category(), "Unexpected file descriptor indicated by select()");
        }
        return true;
    }

    std::string Receive()
    {
        std::ostringstream result{};
        for (;;)
        {
            char buf[4096];

            if (!CanRead())
            {
                break;
            }

            ssize_t bytes = read(sock_, buf, sizeof(buf));

            if (bytes < 0)
            {
                throw std::system_error(errno, std::system_category(), "Failed to read response");
            }
            else if (bytes == 0)
            {
                break;
            }
            result.write(buf, bytes);
        }
        return result.str();
    }
};

std::string CreateRequest(std::string_view hostname)
{
    std::ostringstream ost;
    ost << "GET /testing_safari_getpeername.html HTTP/1.1\r\n"
        "Host: " << hostname << "\r\n"
        "Accept: */*\r\n"
        "User-Agent: sophos_peername_tester\r\n"
        "\r\n";
    return ost.str();
}

int main(int argc, char* argv[])
{
    std::string remoteHost = "www.sophostest.com";

    if (argc > 1)
    {
        remoteHost = argv[1];
    }

    try
    {
        Connection conn{};

        conn.Connect(remoteHost);

        std::cout << "Remote address is " << conn.GetPeerName() << std::endl;;

        auto request = CreateRequest(remoteHost);
        conn.Send(request);
        std::cerr << "Sent request: \n" << request << std::endl;
        auto response = conn.Receive();
        std::cerr << "\nReceived response: " << response << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
