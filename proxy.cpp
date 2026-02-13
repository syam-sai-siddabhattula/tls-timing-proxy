#include <iostream>
#include <cstring>
#include <chrono>
#include <arpa/inet.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/select.h>

#define BUFFER_SIZE 4096
#define LISTEN_PORT 8443

using Clock = std::chrono::high_resolution_clock;

void init_openssl() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

SSL_CTX* create_server_context(const char* cert, const char* key) {
    const SSL_METHOD* method = TLS_server_method();
    SSL_CTX* ctx = SSL_CTX_new(method);
    SSL_CTX_use_certificate_file(ctx, cert, SSL_FILETYPE_PEM);
    SSL_CTX_use_PrivateKey_file(ctx, key, SSL_FILETYPE_PEM);
    return ctx;
}

SSL_CTX* create_client_context() {
    const SSL_METHOD* method = TLS_client_method();
    return SSL_CTX_new(method);
}

int create_server_socket() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(LISTEN_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    listen(sock, 1);

    return sock;
}

int connect_to_server(const char* server_ip, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, server_ip, &addr.sin_addr);

    connect(sock, (struct sockaddr*)&addr, sizeof(addr));
    return sock;
}

int main() {

    init_openssl();

    SSL_CTX* server_ctx = create_server_context("cert.pem", "key.pem");
    SSL_CTX* client_ctx = create_client_context();

    int server_sock = create_server_socket();
    std::cout << "Waiting for client...\n";

    int client_sock = accept(server_sock, nullptr, nullptr);

    SSL* ssl_server = SSL_new(server_ctx);
    SSL_set_fd(ssl_server, client_sock);
    SSL_accept(ssl_server);

    std::cout << "Client connected.\n";

    int remote_sock = connect_to_server("127.0.0.1", 9443);

    SSL* ssl_client = SSL_new(client_ctx);
    SSL_set_fd(ssl_client, remote_sock);
    SSL_connect(ssl_client);

    std::cout << "Connected to backend server.\n";

    char buffer[BUFFER_SIZE];

    auto file_start = Clock::now();

    while (true) {

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(client_sock, &readfds);
        FD_SET(remote_sock, &readfds);

        int maxfd = std::max(client_sock, remote_sock) + 1;

        // WAITING TIME (Idle time)
        
        auto wait_start = Clock::now();
        int activity = select(maxfd, &readfds, NULL, NULL, NULL);
        auto wait_end = Clock::now();

        if (activity <= 0)
            break;

        auto waiting_time =
            std::chrono::duration_cast<std::chrono::microseconds>(
                wait_end - wait_start).count();

        // CLIENT toSERVER
        if (FD_ISSET(client_sock, &readfds)) {

            auto total_start = Clock::now();

            // Decrypt
            auto decrypt_start = Clock::now();
            int bytes = SSL_read(ssl_server, buffer, BUFFER_SIZE);
            auto decrypt_end = Clock::now();
            if (bytes <= 0) break;

            // Encrypt
            auto encrypt_start = Clock::now();
            SSL_write(ssl_client, buffer, bytes);
            auto encrypt_end = Clock::now();

            auto decrypt_time =
                std::chrono::duration_cast<std::chrono::microseconds>(
                    decrypt_end - decrypt_start).count();

            auto encrypt_time =
                std::chrono::duration_cast<std::chrono::microseconds>(
                    encrypt_end - encrypt_start).count();

            auto total =
                std::chrono::duration_cast<std::chrono::microseconds>(
                    encrypt_end - total_start).count();

            std::cout << "----- CLIENT to SERVER -----\n";
            std::cout << "Bytes: " << bytes << "\n";
            std::cout << "Waiting time: " << waiting_time << " us\n";
            std::cout << "Decrypt time: " << decrypt_time << " us\n";
            std::cout << "Encrypt time: " << encrypt_time << " us\n";
            std::cout << "Total forward processing: " << total << " us\n\n";
        }

        // SERVER to CLIENT
        if (FD_ISSET(remote_sock, &readfds)) {

            auto total_start = Clock::now();

            // Decrypt
            auto decrypt_start = Clock::now();
            int bytes = SSL_read(ssl_client, buffer, BUFFER_SIZE);
            auto decrypt_end = Clock::now();
            if (bytes <= 0) break;

            // Encrypt
            auto encrypt_start = Clock::now();
            SSL_write(ssl_server, buffer, bytes);
            auto encrypt_end = Clock::now();

            auto decrypt_time =
                std::chrono::duration_cast<std::chrono::microseconds>(
                    decrypt_end - decrypt_start).count();

            auto encrypt_time =
                std::chrono::duration_cast<std::chrono::microseconds>(
                    encrypt_end - encrypt_start).count();

            auto total =
                std::chrono::duration_cast<std::chrono::microseconds>(
                    encrypt_end - total_start).count();

            std::cout << "----- SERVER toCLIENT -----\n";
            std::cout << "Bytes: " << bytes << "\n";
            std::cout << "Waiting time: " << waiting_time << " us\n";
            std::cout << "Decrypt time: " << decrypt_time << " us\n";
            std::cout << "Encrypt time: " << encrypt_time << " us\n";
            std::cout << "Total reverse processing: " << total << " us\n\n";
        }
    }

    auto file_end = Clock::now();

    auto total_file_time =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            file_end - file_start).count();

    std::cout << "---------------------------------------\n";
    std::cout << "Total file transfer time: "
              << total_file_time << " ms\n";

    SSL_free(ssl_server);
    SSL_free(ssl_client);
    close(client_sock);
    close(remote_sock);
    close(server_sock);

    SSL_CTX_free(server_ctx);
    SSL_CTX_free(client_ctx);
    EVP_cleanup();

    return 0;
}