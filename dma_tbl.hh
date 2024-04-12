#include <vector>
#include <algorithm>
#include <inttypes.h>

using std::vector, std::pair, std::make_pair, std::upper_bound;

namespace vt {

/////////////////////////////////////////////////////////////////////
////////////////// Dynamic memory allocation table //////////////////
/////////////////////////////////////////////////////////////////////
vector<pair<uint64_t, uint64_t>> dma_tbl;   // address, size

void dma_tbl_init() {
    dma_tbl.push_back(make_pair(0, 0));
}

void dma_tbl_insert(void* addr, size_t sz) {
    if (addr == NULL) return;
    auto i = make_pair((uint64_t)addr, (uint64_t)sz);
    dma_tbl.insert(upper_bound(dma_tbl.begin(), dma_tbl.end(), i), i);
}

uint64_t dma_tbl_find(void* addr) {
    if (addr == NULL) return 0;
    uint64_t address = (uint64_t)addr;
    auto i = upper_bound(dma_tbl.begin(), dma_tbl.end(), make_pair(address, UINT64_MAX)) - 1;
    if (i->first <= address && address < i->first + i->second)
        return i->second;
    return 0;
}

void dma_tbl_delete(void* addr) {
    if (addr == NULL) return;
    uint64_t address = (uint64_t)addr;
    auto i = upper_bound(dma_tbl.begin(), dma_tbl.end(), make_pair(address, UINT64_MAX)) - 1;
    if (i->first == address)
        dma_tbl.erase(i);
}
}
