#include "http_fetch.h"
#include "utils/log.h"
#include <sstream>
#include <istream>

static const char* TAG = "ahohc_http";
static constexpr std::string_view HTTP_OK = "HTTP/1.1 200 OK\r";

namespace ahohc::http {
Context::Context(const std::string_view& host, const std::string_view& port)
    : socket(io_context), host(host), port(port)
{
    asio::ip::tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve(host, port);
    asio::connect(socket, endpoints);
}

Header::Header(const HttpOP& op,
               const std::string_view& url, 
               const std::string_view& host,
               const ConnectionType& connection) 
    : op(op), url(url), host(host), connection(connection) {}

std::string Header::to_str() const {
    std::stringstream ss;
    switch(op) {
        case HttpOP::GET:
            ss << "GET ";
            break;
        case HttpOP::POST:
            ss << "POST ";
            break;
        case HttpOP::DELETE:
            ss << "DELETE ";
            break;
        default:
            AHOHC_LOG(LOG_ERROR, TAG, "unknown HttpOP: %{public}d", static_cast<int>(op));
            return "";
    }
    ss << url << " HTTP/1.1\r\n";
    ss << "Host: " << host << "\r\n";
    ss << "Connection: " << (connection == ConnectionType::close ? "close" : "keep-alive") << "\r\n";
    ss << "\r\n";
    return ss.str();
}

std::string Context::http_send(const Header& header) {
    try {
        // 1. 发送 HTTP 请求报文
        std::string request = header.to_str();
        asio::write(socket, asio::buffer(request));

        // 2. 读取响应头部分：以 "\r\n\r\n" 为标识结束
        asio::streambuf responseBuffer;
        asio::read_until(socket, responseBuffer, "\r\n\r\n");

        std::istream responseStream(&responseBuffer);
        std::string statusLine;
        std::getline(responseStream, statusLine);
        std::string responseHeader = statusLine + "\n";

        // 解析响应头，检测是否为分块传输，以及是否提供了 Content-Length 字段
        bool isChunked = false;
        size_t contentLength = 0;
        std::string headerLine;
        while (std::getline(responseStream, headerLine) && headerLine != "\r") {
            responseHeader += headerLine + "\n";
            // 检查是否使用分块传输，注意 HTTP header 对大小写不敏感
            if (headerLine.find("Transfer-Encoding:") != std::string::npos &&
                headerLine.find("chunked") != std::string::npos) {
                isChunked = true;
            }
            // 检查是否存在 Content-Length 字段
            if (headerLine.find("Content-Length:") != std::string::npos) {
                auto pos = headerLine.find(":");
                if (pos != std::string::npos) {
                    // 去除冒号后的空白再转换为数字
                    contentLength = std::stoul(headerLine.substr(pos + 1));
                }
            }
        }

        std::string body;
        // 3. 根据 HTTP 协议，读取响应体部分
        if (isChunked) {
            // 处理分块传输：不断读取 chunk，直到遇到 size 为 0 的 chunk
            while (true) {
                // 读取一行：chunk 大小（以16进制表示）
                std::string chunkSizeLine;
                if (!std::getline(responseStream, chunkSizeLine)) {
                    // 如果流中没有足够数据，则继续从 socket 读取
                    asio::read_until(socket, responseBuffer, "\r\n");
                    std::getline(responseStream, chunkSizeLine);
                }
                // 去除可能的 '\r' 字符
                if (!chunkSizeLine.empty() && chunkSizeLine.back() == '\r') {
                    chunkSizeLine.pop_back();
                }
                if(chunkSizeLine.empty())
                    continue;  // 跳过可能出现的空行

                // 将16进制字符串转换为整数
                std::istringstream chunkSizeStream(chunkSizeLine);
                size_t chunkSize = 0;
                chunkSizeStream >> std::hex >> chunkSize;
                if (chunkSize == 0) {
                    // 读到最后一个 chunk（0 表示结束）
                    // 可选：读取 trailer header（末尾可能还有额外 header，这里简单丢弃）
                    std::getline(responseStream, headerLine); // 读取后面的CRLF
                    break;
                }
                // 确保缓冲区中已经有 chunkSize 个字节，如果不足则继续从 socket 读取
                if(responseBuffer.size() < chunkSize) {
                    asio::read(socket, responseBuffer,
                        asio::transfer_exactly(chunkSize - responseBuffer.size()));
                }
                // 从缓冲区中取出 chunkSize 个字节
                std::vector<char> chunkData(chunkSize);
                responseStream.read(&chunkData[0], chunkSize);
                body.append(chunkData.data(), chunkSize);
                // 再读取紧跟 chunk 后的那组 CRLF
                asio::read_until(socket, responseBuffer, "\r\n");
                std::getline(responseStream, headerLine);  // 丢弃这一行（实际上只包含CRLF）
            }
        }
        else if (contentLength > 0) {
            // 如果有 Content-Length，则读取剩余的 contentLength 个字节
            size_t bytesInBuffer = responseBuffer.size();
            if (bytesInBuffer < contentLength) {
                asio::read(socket, responseBuffer,
                    asio::transfer_exactly(contentLength - bytesInBuffer));
            }
            std::ostringstream oss;
            oss << &responseBuffer;
            body = oss.str();
        }
        else {
            // 如果既没有分块也没有 Content-Length，则采用直到 socket 关闭的方式（这种情况较少见）
            while (asio::read(socket, responseBuffer, asio::transfer_at_least(1)))
                ;
            std::ostringstream oss;
            oss << &responseBuffer;
            body = oss.str();
        }

        // 返回完整的 HTTP 响应（头+体），开发时可按需进一步分离
        return responseHeader + "\r\n" + body;
    }
    catch (std::exception& e) {
        AHOHC_LOG(LOG_ERROR, TAG, "Error during HTTP request: %{public}s", e.what());
        return "";
    }
}

std::string Context::http_send(const Header& header, const std::string_view& content) {
    try {
        // 【1】构造请求报文：利用 Header::to_str() 得到基础头部，然后补充 Content-Length 字段和请求体
        std::string baseRequest = header.to_str();
        // Header::to_str() 通常以 "\r\n\r\n" 结尾，去掉末尾的 "\r\n\r\n"
        if (baseRequest.size() >= 4 && baseRequest.substr(baseRequest.size() - 4) == "\r\n\r\n") {
            baseRequest.erase(baseRequest.size() - 4);
        }
        std::stringstream requestStream;
        requestStream << baseRequest << "\r\n"
                      << "Content-Length: " << content.size() << "\r\n"
                      << "\r\n"
                      << content;
        std::string requestStr = requestStream.str();
        asio::write(socket, asio::buffer(requestStr));

        // 【2】读取响应头，直到遇到 "\r\n\r\n"
        asio::streambuf responseBuffer;
        asio::read_until(socket, responseBuffer, "\r\n\r\n");

        std::istream responseStream(&responseBuffer);
        std::string statusLine;
        std::getline(responseStream, statusLine);
        // 去除状态行末尾的'\r'（因为HTTP行结束符为"\r\n"）
        if (!statusLine.empty() && statusLine.back() == '\r')
            statusLine.pop_back();

        //【3】在这里检查 HTTP 状态码（注意：如果未去除'\r', 则比较会失败）
        if (statusLine != "HTTP/1.1 200 OK") {
            AHOHC_LOG(LOG_WARN, TAG, "HTTP request failed with status: %{public}s", statusLine.c_str());
            // 根据需要，可以选择直接返回空字符串或者后续处理错误
            // 这里直接返回整个响应（头+体），以便上层可进一步检查
        }
        std::string responseHeader = statusLine + "\n";

        // 解析后续响应头行，检测是否为分块传输以及解析 Content-Length 字段
        bool isChunked = false;
        size_t contentLength = 0;
        std::string headerLine;
        while (std::getline(responseStream, headerLine) && headerLine != "\r") {
            // 同样处理去除尾部的'\r'
            if (!headerLine.empty() && headerLine.back() == '\r')
                headerLine.pop_back();
            responseHeader += headerLine + "\n";

            // 判断是否为分块传输（不区分大小写）
            if (headerLine.find("Transfer-Encoding:") != std::string::npos &&
                headerLine.find("chunked") != std::string::npos)
            {
                isChunked = true;
            }
            // 检查 Content-Length 字段
            if (headerLine.find("Content-Length:") != std::string::npos) {
                auto pos = headerLine.find(":");
                if (pos != std::string::npos) {
                    contentLength = std::stoul(headerLine.substr(pos + 1));
                }
            }
        }

        // 【4】读取响应体：根据是否是分块传输或Content-Length来处理
        std::string body;
        if (isChunked) {
            // 按 chunked 模式读取：循环读取每一个 chunk
            while (true) {
                std::string chunkSizeLine;
                // 如果现有流中数据不足则继续读取
                if (!std::getline(responseStream, chunkSizeLine)) {
                    asio::read_until(socket, responseBuffer, "\r\n");
                    std::getline(responseStream, chunkSizeLine);
                }
                if (!chunkSizeLine.empty() && chunkSizeLine.back() == '\r')
                    chunkSizeLine.pop_back();
                if (chunkSizeLine.empty())
                    continue;  // 跳过空行

                // 将16进制数字转为整数
                std::istringstream chunkSizeStream(chunkSizeLine);
                size_t chunkSize = 0;
                chunkSizeStream >> std::hex >> chunkSize;
                if (chunkSize == 0) {
                    // 最后一个 chunk
                    std::getline(responseStream, headerLine); // 丢弃末尾的 CRLF
                    break;
                }
                // 保证缓冲区中有足够的 chunk 数据
                if (responseBuffer.size() < chunkSize) {
                    asio::read(socket, responseBuffer,
                        asio::transfer_exactly(chunkSize - responseBuffer.size()));
                }
                std::vector<char> chunkData(chunkSize);
                responseStream.read(chunkData.data(), chunkSize);
                body.append(chunkData.data(), chunkSize);
                // 读取每个 chunk 后面的 CRLF
                asio::read_until(socket, responseBuffer, "\r\n");
                std::getline(responseStream, headerLine); // 丢弃这一行
            }
        }
        else if (contentLength > 0) {
            // 非分块传输：读取 Content-Length 指定长度数据
            size_t bytesInBuffer = responseBuffer.size();
            if (bytesInBuffer < contentLength) {
                asio::read(socket, responseBuffer,
                    asio::transfer_exactly(contentLength - bytesInBuffer));
            }
            std::ostringstream oss;
            oss << &responseBuffer;
            body = oss.str();
        }
        else {
            // 兜底策略：一直读取直到 socket 关闭
            while (asio::read(socket, responseBuffer, asio::transfer_at_least(1))) { }
            std::ostringstream oss;
            oss << &responseBuffer;
            body = oss.str();
        }

        // 拼接响应头和响应体返回
        return responseHeader + "\r\n" + body;
    }
    catch (std::exception& e) {
        AHOHC_LOG(LOG_ERROR, TAG, "Error during HTTP request: %{public}s", e.what());
        return "";
    }
}

std::string Context::http_get(const std::string_view& url,ConnectionType connection_type) {
    Header header(HttpOP::GET, url, host, connection_type);
    auto response = http_send(header);
    std::istringstream iss(response);
    std::string first_line;
    std::getline(iss, first_line);
    if(first_line != HTTP_OK) {
        AHOHC_LOG(LOG_WARN, TAG, "http get failed, response: %{public}s", response.c_str());
    }
    else {
        AHOHC_LOG(LOG_DEBUG, TAG, "http get from %{public}s success", url.data());
    }
    return response;
}

std::string Context::http_get(const std::string_view& url,
    const std::string_view& host,ConnectionType connection_type) {
    Header header(HttpOP::GET, url, host, connection_type);
    auto response = http_send(header);
    std::istringstream iss(response);
    std::string first_line;
    std::getline(iss, first_line);
    if(first_line != HTTP_OK) {
        AHOHC_LOG(LOG_WARN, TAG, "http get failed, response: %{public}s", first_line.c_str());
    }
    else {
        AHOHC_LOG(LOG_DEBUG, TAG, "http get from %{public}s success", url.data());
    }
    return response;
}

void Context::http_post(const std::string_view& url, const std::string_view& content) {
    Header header(HttpOP::POST, url, host, ConnectionType::close);
    auto response = http_send(header, content);
    std::istringstream iss(response);
    std::string first_line;
    std::getline(iss, first_line);
    if(first_line != HTTP_OK) {
        AHOHC_LOG(LOG_WARN, TAG, "http post to %{public}s failed, response: %{public}s", url.data(), first_line.c_str());
    }
    else {
        AHOHC_LOG(LOG_DEBUG, TAG, "http post to %{public}s success", url.data());
    }
}

void Context::http_delete(const std::string_view& url) {
    Header header(HttpOP::DELETE, url, host, ConnectionType::close);
    auto response = http_send(header);
    std::istringstream iss(response);
    std::string first_line;
    std::getline(iss, first_line);
    if(first_line != HTTP_OK) {
        AHOHC_LOG(LOG_WARN, TAG, "http delete at %{public}s failed, response: %{public}s", url.data(), first_line.c_str());
    }
    else {
        AHOHC_LOG(LOG_DEBUG, TAG, "http delete at %{public}s success", url.data());
    }
}
} // namespace ahohc::http
