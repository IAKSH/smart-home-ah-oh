import socket
import json

# 广播目标地址和服务器监听的 UDP 端口
BROADCAST_IP = "255.255.255.255"
UDP_PORT = 8888
# 要发送的广播消息，例如“DISCOVER_SERVER”
MESSAGE = "AHOH_DISCOVER_SERVER".encode("utf-8")

def main():
    # 创建 UDP 套接字
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    # 允许广播
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    # 设置接收超时，单位为秒
    sock.settimeout(5)

    try:
        # 发送广播消息到指定端口
        sock.sendto(MESSAGE, (BROADCAST_IP, UDP_PORT))
        print("Broadcast message sent. Waiting for response...")

        # 等待服务器响应，缓存大小设置为 1024 字节
        data, addr = sock.recvfrom(1024)
        decoded_data = data.decode("utf-8")
        print(f"Received response from {addr[0]}:{addr[1]}")
        print("Raw response:", decoded_data)

        # 尝试将响应解析为 JSON
        response_json = json.loads(decoded_data)
        print("Parsed JSON response:")
        print(response_json)

    except socket.timeout:
        print("Timed out waiting for a response.")
    except Exception as e:
        print(f"An error occurred: {e}")
    finally:
        sock.close()

if __name__ == "__main__":
    main()
