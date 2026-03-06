#include <iostream>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>

#define SERVER_IP "165.134.13.34"   // Change if needed
#define SERVER_PORT 8443

#define LOGICAL_RUNS 150           // Repeat runs for averaging
#define START_PAYLOAD 128
#define MAX_PAYLOAD 3000
#define STEP 128

void init_openssl() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

int main() {

    init_openssl();

    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Connect failed");
        return 1;
    }

    SSL* ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sock);

    if (SSL_connect(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        return 1;
    }

    std::cout << "TLS Handshake Completed (Only Once)\n\n";

    // ===== Logical Runs =====
    for (int run = 1; run <= LOGICAL_RUNS; run++) {

        size_t payload = START_PAYLOAD;

        while (payload <= MAX_PAYLOAD) {

            std::vector<char> buffer(payload, 'A');

            int written = SSL_write(ssl, buffer.data(), buffer.size());
            if (written <= 0) {
                std::cerr << "SSL_write failed\n";
                goto cleanup;
            }

            payload += STEP;
        }
    }

cleanup:

    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(sock);
    SSL_CTX_free(ctx);

    return 0;
}
