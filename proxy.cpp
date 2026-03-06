#include <iostream>
#include <cstring>
#include <chrono>
#include <arpa/inet.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <fstream>

#define BUFFER_SIZE 4096
#define LISTEN_PORT 8443

#define START_PAYLOAD 128
#define MAX_PAYLOAD 3000
#define STEP 128

using Clock = std::chrono::high_resolution_clock;

void init_openssl() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

SSL_CTX* create_server_context(const char* cert, const char* key) {
    SSL_CTX* ctx = SSL_CTX_new(TLS_server_method());
    SSL_CTX_use_certificate_file(ctx, cert, SSL_FILETYPE_PEM);
    SSL_CTX_use_PrivateKey_file(ctx, key, SSL_FILETYPE_PEM);
    return ctx;
}

SSL_CTX* create_client_context() {
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
    return ctx;
}

int main() {

    init_openssl();

    std::ofstream logfile("steady_state_results.csv");
    logfile << "direction,bytes,decrypt_us,encrypt_us,total_us,read_calls\n";
    logfile.flush();

    SSL_CTX* server_ctx = create_server_context("cert.pem", "key.pem");
    SSL_CTX* client_ctx = create_client_context();

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(LISTEN_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_sock, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_sock, 1);

    std::cout << "Proxy ready...\n";

    int client_sock = accept(server_sock, nullptr, nullptr);

    SSL* ssl_server = SSL_new(server_ctx);
    SSL_set_fd(ssl_server, client_sock);
    SSL_accept(ssl_server);

    int backend_sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in backend_addr{};
    backend_addr.sin_family = AF_INET;
    backend_addr.sin_port = htons(9443);
    inet_pton(AF_INET, "127.0.0.1", &backend_addr.sin_addr);
    connect(backend_sock, (struct sockaddr*)&backend_addr, sizeof(backend_addr));

    SSL* ssl_client = SSL_new(client_ctx);
    SSL_set_fd(ssl_client, backend_sock);
    SSL_connect(ssl_client);

    char buffer[BUFFER_SIZE];

    size_t expected_payload = START_PAYLOAD;

    while (true) {

        size_t total_bytes = 0;
        long decrypt_total = 0;
        long encrypt_total = 0;
        int read_calls = 0;

        auto total_start = Clock::now();

        while (total_bytes < expected_payload) {

            auto decrypt_start = Clock::now();
            int bytes = SSL_read(ssl_server, buffer, BUFFER_SIZE);
            auto decrypt_end = Clock::now();

            if (bytes <= 0)
                goto cleanup;

            read_calls++;

            decrypt_total += std::chrono::duration_cast<std::chrono::microseconds>(
                decrypt_end - decrypt_start).count();

            auto encrypt_start = Clock::now();
            SSL_write(ssl_client, buffer, bytes);
            auto encrypt_end = Clock::now();

            encrypt_total += std::chrono::duration_cast<std::chrono::microseconds>(
                encrypt_end - encrypt_start).count();

            total_bytes += bytes;
        }

        auto total_end = Clock::now();
        long total_time =
            std::chrono::duration_cast<std::chrono::microseconds>(
                total_end - total_start).count();

        logfile << "forward,"
                << expected_payload << ","
                << decrypt_total << ","
                << encrypt_total << ","
                << total_time << ","
                << read_calls << "\n";

        logfile.flush();

        // Move to next payload size
        expected_payload += STEP;

        if (expected_payload > MAX_PAYLOAD)
            expected_payload = START_PAYLOAD;
    }

cleanup:

    SSL_shutdown(ssl_server);
    SSL_shutdown(ssl_client);
    SSL_free(ssl_server);
    SSL_free(ssl_client);
    close(client_sock);
    close(backend_sock);
    close(server_sock);
    SSL_CTX_free(server_ctx);
    SSL_CTX_free(client_ctx);

    return 0;
}
