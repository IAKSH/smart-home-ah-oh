//
// Created on 2025/4/8.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef HMOS_CPP_TEST_UDP_BROADCAST_H
#define HMOS_CPP_TEST_UDP_BROADCAST_H

#include <asio.hpp>
#include <array>
#include <chrono>
#include <stdexcept>
#include <string>
#include <string_view>
#include "utils/log.h"

namespace ahohc::udp {
using namespace asio;

static constexpr char* TAG = "ahohc_udp";

class Context {
public:
    Context(const std::string_view& ip, unsigned short port)
        : ip(ip),
          port(port),
          socket(io_context),
          // 这里如果需要使用随机的本地端口，可以将 port 改为 0：
          // local_endpoint(ip::udp::endpoint(ip::address_v4::any(), 0))
          local_endpoint(ip::udp::endpoint(ip::address_v4::any(), port)),
          target_endpoint(ip::udp::endpoint(ip::make_address(ip), port))
    {
        socket.open(ip::udp::v4());
        socket.set_option(socket_base::reuse_address(true));
        socket.bind(local_endpoint);
        socket.set_option(socket_base::broadcast(true));
    }

    // 发送函数：仅负责同步地发送UDP广播消息
    void send(const std::string_view& msg) {
        try {
            socket.send_to(buffer(msg.data(), msg.size()), target_endpoint);
        } catch (const std::exception& e) {
            AHOHC_LOG(LOG_ERROR, TAG,
                      "exception caught when sending udp to %{public}s:%{public}d, error: %{public}s",
                      ip.c_str(), port, e.what());
        }
    }

    // 接收函数：通过异步操作和定时器等待响应数据
    std::string receive(std::chrono::milliseconds timeout = std::chrono::milliseconds(3000)) {
        try {
            std::string response;
            std::array<char, 1024> recv_buf{};
            ip::udp::endpoint sender_endpoint;
            bool receive_completed = false;  // 是否完成接收
            bool timeout_occurred = false;     // 是否超时
            std::error_code receive_error;
            std::size_t bytes_received = 0;
    
            // 1. 启动异步接收
            socket.async_receive_from(buffer(recv_buf), sender_endpoint,
                [&](const std::error_code& ec, std::size_t bytes_transferred) {
                    receive_error = ec;
                    bytes_received = bytes_transferred;
                    receive_completed = true;
                }
            );
    
            // 2. 设置定时器实现超时控制
            steady_timer timer(io_context);
            timer.expires_after(timeout);
            timer.async_wait([&](const std::error_code &ec) {
                if (!receive_completed) {
                    timeout_occurred = true;
                    socket.cancel();  // 取消等待操作
                }
            });
    
            // 3. 运行io_context，直到异步接收或超时事件完成
            io_context.run();
            io_context.restart();  // 重置io_context，便于后续使用
    
            // 4. 检查超时和接收错误
            if (timeout_occurred) {
                throw std::runtime_error("Timeout waiting for response");
            }
            if (receive_error && receive_error != error::operation_aborted) {
                throw std::runtime_error("Receive error: " + receive_error.message());
            }
            // 5. 构造响应字符串
            response.assign(recv_buf.data(), bytes_received);
            return response;
        }
        catch(std::exception& e) {
            AHOHC_LOG(LOG_ERROR, TAG,
                "exception caught when receiving udp from %{public}s:%{public}d, error: %{public}s",
                ip.c_str(),port,e.what());
        }
    }

private:
    io_context io_context;
    ip::udp::socket socket; 
    ip::udp::endpoint local_endpoint;
    ip::udp::endpoint target_endpoint;
    std::string ip;
    short port;
};

}  // namespace ahohc::udp

#endif //HMOS_CPP_TEST_UDP_BROADCAST_H
