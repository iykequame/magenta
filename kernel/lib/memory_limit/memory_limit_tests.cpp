#include <err.h>
#include <lib/memory_limit.h>
#include <sys/types.h>
#include <unittest.h>

typedef struct {
    uintptr_t base;
    size_t size;
} test_range_t;

// Memory map provided by EFI 5/3/17
static test_range_t nuc_ranges[] = {
    {
        0, 0x58000,
    },
    {0x59000, 0x45000},
    {0x100000, 0x85d8b000},
    {0x85eb6000, 0x4375000},
    {0x8b2ff000, 0x1000},
    {0x100000000, 0x36f000000},
};

static mem_limit_cfg_t nuc_cfg = {
    .kernel_base = 0x100000,
    .kernel_size = 4 * MB,
    .ramdisk_base = 0x818e4000,
    .ramdisk_size = 4 * MB,
    .memory_limit = 0,
    .found_kernel = 0,
    .found_ramdisk = 0,
};

// Memory map provided by EFI 5/3/17
static test_range_t acer12_ranges[] = {
    {0, 0x58000},
    {0x59000, 0x2d000},
    {0x100000, 0x7359d000},
    {0x736c8000, 0xb1000},
    {0x74079000, 0x16215000},
    {0x8aefe000, 0x1000},
    {0x100000000, 0x6f000000},
};

static mem_limit_cfg_t acer12_cfg = {
    .kernel_base = 0x100000,
    .kernel_size = 4u * MB,
    .ramdisk_base = 0x71b20000,
    .ramdisk_size = 4u * MB,
    .memory_limit = 0,
    .found_kernel = 0,
    .found_ramdisk = 0,
};

static test_range_t rpi3_ranges[] = {
    {0xffff000000000000, 0x20000000},
};

static mem_limit_cfg_t rpi3_cfg = {
    .kernel_base = 0xffff000000000000,
    .kernel_size = 4 * MB,
    .ramdisk_base = 0xffff000007d44000,
    .ramdisk_size = 3u * MB,
    .memory_limit = 0,
    .found_kernel = 0,
    .found_ramdisk = 0,
};

typedef struct {
    mem_limit_cfg_t cfg;
    test_range_t* ranges;
    size_t range_cnt;
} platform_test_cases_t;

// TODO: These tests ensure that we segment things up and find the kernel
// and ramdisk, but they don't really cover any interesting cases. They're
// also presently bounded by rpi3's 512MB memory ceiling. They will be more
// useful if some specific cases are written, but those should be easily
// addable as we find new problems down the line.
static bool ml_test_platforms(void* context) {
    BEGIN_TEST;
    // This test closely matches the observed layouts for NUC/Acer12/rpi3
    iovec_t vecs[2];
    status_t status;
    size_t used;
    platform_test_cases_t test_cases[] = {
        {nuc_cfg, nuc_ranges, countof(nuc_ranges)},
        {acer12_cfg, acer12_ranges, countof(acer12_ranges)},
        {rpi3_cfg, rpi3_ranges, countof(rpi3_ranges)},
    };

    for (auto& test : test_cases) {
        auto cfg = test.cfg;
        for (size_t memory_limit = 512 * MB; memory_limit >= 32 * MB; memory_limit /= 2) {
            size_t size = 0;
            cfg.memory_limit = memory_limit;
            cfg.found_kernel = false;
            cfg.found_ramdisk = false;

            for (size_t i = 0; i < test.range_cnt; i++) {
                status = mem_limit_apply(&cfg, test.ranges[i].base, test.ranges[i].size, vecs, countof(vecs), &used);
                EXPECT_EQ(NO_ERROR, status, "checking status");
                for (size_t i = 0; i < used; i++) {
                    size += vecs[i].iov_len;
                }
            }

            EXPECT_EQ(true, cfg.found_kernel, "checking kernel");
            EXPECT_EQ(true, cfg.found_ramdisk, "checking ramdisk");
            EXPECT_EQ(memory_limit, size, "comparing limit");
        }
    }
    END_TEST;
}

static bool ml_test_fat_ramdisk(void* context) {
    BEGIN_TEST;
    mem_limit_cfg_t cfg = nuc_cfg;
    size_t memory_limit = 64 * MB;
    size_t size = 0;
    cfg.ramdisk_size = 64 * MB;
    cfg.memory_limit = memory_limit;
    iovec_t vecs[2];
    size_t used;

    for (size_t i = 0; i < countof(nuc_ranges); i++) {
        status_t status = mem_limit_apply(&cfg, nuc_ranges[i].base, nuc_ranges[i].size, vecs, countof(vecs), &used);
        EXPECT_EQ(NO_ERROR, status, "checking status");
        for (size_t i = 0; i < used; i++) {
            size += vecs[i].iov_len;
        }
    }

    EXPECT_EQ(true, cfg.found_kernel, "checking kernel");
    EXPECT_EQ(true, cfg.found_ramdisk, "checking ramdisk");
    EXPECT_NEQ(memory_limit, size, "checking that size and limit don't match");
    EXPECT_EQ(cfg.kernel_size + cfg.ramdisk_size, size, "checking the limit grew to fit kernel + ramdisk");
    END_TEST;
}

#define ML_UNITTEST(fname) UNITTEST(#fname, fname)
UNITTEST_START_TESTCASE(memlimit_tests)
ML_UNITTEST(ml_test_platforms)
ML_UNITTEST(ml_test_fat_ramdisk)
UNITTEST_END_TESTCASE(memlimit_tests, "memlim_tests", "Memory limit tests", nullptr, nullptr);
#undef ML_UNITTEST
