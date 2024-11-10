#include <cblas.h>
#include <iostream>

int main()
{
    // 2x2 Matrix
    int M = 2, N = 2, K = 2;
    double A[4] = {1.0, 2.0, 3.0, 4.0};
    double B[4] = {5.0, 6.0, 7.0, 8.0};
    double C[4];

    // General Matrix-Matrix multiplication
    cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
        M, N, K,
        1.0, A, K, B, N,
        0.0, C, N);

    // Print results
    std::cout << "Result of A * B = C:" << std::endl;
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            std::cout << C[i * 2 + j] << " ";
        }
        std::cout << std::endl;
    }
    return 0;
}

// CompileOptions: `pkg-config --cflags --libs openblas`
