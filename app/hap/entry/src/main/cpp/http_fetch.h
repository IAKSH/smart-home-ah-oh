#ifndef HTTP_FETCH_H
#define HTTP_FETCH_H

#include <asio.hpp>
#include <string>
#include <string_view>

namespace ahohc::http {
enum class HttpOP {
    GET,
    POST,
    DELETE
};

enum class ConnectionType {
    close,
    keep_alive
};

struct Header {
    HttpOP op;
    std::string url;
    std::string host;
    ConnectionType connection;

    Header(const HttpOP& op,
           const std::string_view& url, 
           const std::string_view& host,
           const ConnectionType& connection);
    
    std::string to_str() const;
};

class Context {
private:
    asio::io_context io_context;
    asio::ip::tcp::socket socket;
    std::string host;
    std::string port;
    
public:
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;
    
    Context(const std::string_view& host, const std::string_view& port);

    std::string http_send(const Header& header);
    std::string http_send(const Header& header, const std::string_view& content);
    
    std::string http_get(const std::string_view& url,
                         ConnectionType connection_type = ConnectionType::keep_alive);
    std::string http_get(const std::string_view& url,
                         const std::string_view& host,
                         ConnectionType connection_type = ConnectionType::keep_alive);
    
    void http_post(const std::string_view& url, const std::string_view& content);
    void http_delete(const std::string_view& url);
};
} // namespace ahohc::http
#endif // HTTP_FETCH_H
