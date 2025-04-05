#pragma once

#include <cstdint>
#include <string>

namespace ahohs::udp_server {

class UdpResponder {
 public:
    /**
     * 构造函数
     *
     * @param server_ip         服务器自身的 IP 地址，用于回复
     * @param server_port       服务器 HTTP 服务端口，用于回复
     * @param mqtt_broker_ip    MQTT Broker 的 IP 地址，用于回复
     * @param mqtt_broker_port  MQTT Broker 的端口，用于回复
     * @param port              监听的 UDP 端口（默认 8888）
     */
    UdpResponder(const std::string& server_ip,
                 uint16_t server_port,
                 const std::string& mqtt_broker_ip,
                 uint16_t mqtt_broker_port,
                 uint16_t port = 8888);
    ~UdpResponder();

    UdpResponder(const UdpResponder&) = delete;
    UdpResponder& operator=(const UdpResponder&) = delete;
    UdpResponder(UdpResponder&&) noexcept = default;
    UdpResponder& operator=(UdpResponder&&) noexcept = default;

    /// 启动 UDP 监听与回复，循环处理接收到的广播消息
    void start();

 private:
    std::string server_ip;
    uint16_t server_port;
    std::string mqtt_broker_ip;
    uint16_t mqtt_broker_port;
    uint16_t port;  // UDP 监听端口
    int sock_fd;

    static constexpr std::string_view UDP_DISCOVER_MSG = "AHOH_DISCOVER_SERVER";

    bool init_socket();
    void listen_and_respond();
};

}  // namespace ahohs::udp_server
