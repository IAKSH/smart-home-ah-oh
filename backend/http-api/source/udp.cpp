#include "udp.h"
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <errno.h>

namespace ahohs::udp_server {

static auto logger = spdlog::stdout_color_mt("udp_responder");

UdpResponder::UdpResponder(const std::string& server_ip,
                           uint16_t server_port,
                           const std::string& mqtt_broker_ip,
                           uint16_t mqtt_broker_port,
                           uint16_t port)
    : server_ip(server_ip),
      server_port(server_port),
      mqtt_broker_ip(mqtt_broker_ip),
      mqtt_broker_port(mqtt_broker_port),
      port(port),
      sock_fd(-1) {
    if (!init_socket()) {
        logger->error("Failed to initialize UDP socket on port {}", port);
    } else {
        logger->info("UDP socket initialized on port {}", port);
    }
}

UdpResponder::~UdpResponder() {
    if (sock_fd >= 0) {
        close(sock_fd);
    }
}

bool UdpResponder::init_socket() {
    // 创建 UDP 套接字
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        logger->error("Socket creation failed: {}", strerror(errno));
        return false;
    }

    // 启用广播选项
    int broadcastEnable = 1;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) < 0) {
        logger->error("Failed to set SO_BROADCAST: {}", strerror(errno));
        return false;
    }

    // 启用地址复用，允许多进程绑定同一端口
    int opt = 1;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        logger->error("Failed to set SO_REUSEADDR: {}", strerror(errno));
        // 该选项失败通常不致命，继续运行
    }

    // 绑定到任意 IP (INADDR_ANY) 和指定端口
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(sock_fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        logger->error("Bind failed: {}", strerror(errno));
        return false;
    }
    return true;
}

void UdpResponder::listen_and_respond() {
    char buffer[1024] = {0};
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // 阻塞等待接收 UDP 消息
    int n = recvfrom(sock_fd, buffer, sizeof(buffer) - 1, 0,
                     reinterpret_cast<struct sockaddr*>(&client_addr), &addr_len);
    if (n < 0) {
        logger->error("recvfrom failed: {}", strerror(errno));
        return;
    }
    buffer[n] = '\0'; // 确保字符串结束
    logger->info("Received UDP message from {}:{}",
                 inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    logger->debug("Message content: {}", buffer);

    if(std::string(buffer) != UDP_DISCOVER_MSG) {
        logger->info("Not UDP Discovery msg, ignored");
        return;
    }

    // 构造回复消息（JSON 格式），包含完整的 IP 和端口信息
    std::string reply = "{\"server_ip\":\"" + server_ip +
                        "\", \"server_port\":\"" + std::to_string(server_port) +
                        "\", \"mqtt_broker_ip\":\"" + mqtt_broker_ip +
                        "\", \"mqtt_broker_port\":\"" + std::to_string(mqtt_broker_port) +
                        "\"}";
    int sent = sendto(sock_fd, reply.c_str(), reply.size(), 0,
                      reinterpret_cast<struct sockaddr*>(&client_addr), addr_len);
    if (sent < 0) {
        logger->error("sendto failed: {}", strerror(errno));
    } else {
        logger->info("Sent response to {}:{}",
                     inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    }
}

void UdpResponder::start() {
    logger->info("Starting UDP responder on port {}", port);
    while (true) {
        listen_and_respond();
    }
}

}  // namespace ahohs::udp_server
