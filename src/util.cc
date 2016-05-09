#include "util.h"

PermutationList DOUBLE_INDICES;

static void Permute(PermutationList &result, IndexList combo)
{
    auto mx = *max_element(begin(combo), end(combo));
    do {
        result.push_back({mx, combo});
    } while (next_permutation(begin(combo), end(combo)));
}

static void Combo(PermutationList &result,
                  IndexList &c,
                  size_t n, size_t offset, size_t k)
{
    if (k == 0) {
        Permute(result, c);
    } else {
        for (size_t i=offset; i <= n-k; ++i) {
            c.push_back(i);
            Combo(result, c, n, i+1, k-1);
            c.pop_back();
        }
    }
}

static void Sort(PermutationList &r) {
    sort(begin(r), end(r),
         [](const auto &a, const auto &b) {
             if (a.first != b.first) {
                 return a.first < b.first;
             }
             auto &x = a.second;
             auto &y = b.second;
             for (int i=0; i < x.size(); ++i) {
                if (x[i] != y[i]) {
                    return x[i] < y[i];
                }
             }
             return false;
         });
}

static PermutationList BuildPermutations(size_t n, size_t k) {
    auto result = PermutationList();
    auto combo = IndexList();

    Combo(result, combo, n, 0, k);
    Sort(result);

    return result;
}


void BuildDoubleIndices() {
    DOUBLE_INDICES = BuildPermutations(MAX_DOUBLE_INDICES, 2);
}
