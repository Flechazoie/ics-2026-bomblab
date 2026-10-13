#include "defuser.h"

#include <inttypes.h>
#include <stdio.h>

#define MAX_FIELDS 8

/* Personal bomb: package 98cfaa6e4ed540536883b8ea.
 * Recovered from its unmodified x86-64 instructions and data tables.
 * All arithmetic below follows the bomb's unsigned 32-bit operations.
 */
static uint32_t rotate_right(uint32_t x, unsigned n) {
    return (x >> n) | (x << (32U - n));
}

static uint32_t phase3_case(unsigned selector, uint32_t x) {
    /* Jump-table order is 0, 4, 5, 6, 3, 2, 7, 1. */
    switch (selector) {
        case 0: return (x * 2U) ^ 0xb1a5U;
        case 1: return (x ^ 0x99d4U) + (x >> 1);
        case 2: return (x * 4U) ^ 0x1ecdU;
        case 3: return x * 5U + 0x24a22U;
        case 4: return x * 5U + 0x73c6U;
        case 5: return x * 5U + 0x42e6eU;
        case 6: return x * 5U + 0xf9eeU;
        case 7: return (x ^ 0x7a98U) + (x >> 1);
        default: return 0;
    }
}

static uint32_t phase4_walk(uint32_t target, uint32_t low,
                            uint32_t high, uint32_t salt) {
    uint32_t mid = low + ((high - low) >> 1);
    if (target == mid) return (mid + 0xccU) ^ salt;
    if (target < mid)
        return (3U * phase4_walk(target, low, mid - 1U, salt))
               ^ (mid + salt) ^ 0x1cU;
    return (3U * phase4_walk(target, mid + 1U, high, salt))
           ^ (mid + salt) ^ 0x0fU;
}

static uint32_t phase6_op(unsigned selector, uint32_t x) {
    /* The eight pointers select operations 5, 0, 1, 2, 3, 2, 3, 4. */
    switch (selector) {
        case 0: return rotate_right(x, 3);
        case 1: return (x ^ 0xd14fU) + 4U * x;
        case 2: return ((x << 5) | (x >> 27)) ^ 0x1a65U;
        case 3:
        case 5: return ((x << 5) + 0xfe9eU) ^ (3U * x);
        case 4:
        case 6: return (x + 0x47a9U) ^ (x >> 5);
        case 7: return (x ^ (x >> 3)) + 0x99c6U;
        default: return 0;
    }
}

/*
 * 把 values 中的 count 个整数写成一行“空格分隔的无符号十进制整数”，存入 out。
 * 成功返回 0；count 为 0（还没有实现）、snprintf 出错或结果被截断时返回非零。
 */
static int write_answer(char *out, size_t cap, const uint32_t *values, size_t count) {
    size_t used = 0;
    for (size_t i = 0; i < count; ++i) {
        int size = snprintf(out + used, cap - used, "%s%" PRIu32, i ? " " : "", values[i]);
        if (size < 0 || (size_t)size >= cap - used) return 1;
        used += (size_t)size;
    }
    return count == 0;
}

int solve_phase_1(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 1;
    uint32_t low = (uint32_t)challenge;
    uint32_t high = (uint32_t)(challenge >> 32);
    uint32_t step2 = (high + 8U * low) ^ 0xe614U;
    uint32_t step0 = (5U * low + (uint32_t)(challenge >> 16)) ^ 0x2e9eU;
    values[0] = 5U * step0 ^ (step2 + 0x597U);
    return write_answer(out, cap, values, count);
}

int solve_phase_2(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 6;
    values[0] = (uint32_t)((challenge >> 27) & 15U);
    for (unsigned i = 1; i < count; ++i) {
        unsigned shift = 7U + 3U * i;
        values[i] = (values[i - 1] + (uint32_t)((challenge >> shift) & 31U))
                    ^ (3U * i);
    }
    return write_answer(out, cap, values, count);
}

int solve_phase_3(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 2;
    values[0] = (uint32_t)(challenge & 7U);
    uint32_t x = (uint32_t)(challenge >> 3) ^ (uint32_t)(challenge >> 32);
    values[1] = phase3_case(values[0], x);
    return write_answer(out, cap, values, count);
}

int solve_phase_4(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 1;
    uint32_t target = (uint32_t)(challenge & 127U);
    uint32_t salt = (uint32_t)(challenge >> 7) ^ (uint32_t)(challenge >> 32);
    values[0] = phase4_walk(target, 0U, 127U, salt);
    return write_answer(out, cap, values, count);
}

int solve_phase_5(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 2;
    /* Node indices encode the pointers at p5_nodes (base 0x405ba0,
     * stride 16); fields are next at +0, identifier at +8, value at +12.
     * Index 12 represents the NULL link, which valid paths never visit.
     */
    static const unsigned next[12] = {1, 6, 12, 7, 9, 0, 8, 10, 3, 11, 2, 5};
    static const uint32_t id[12] = {
        0x374U, 0x877U, 0x116U, 0x70eU, 0x61dU, 0x918U,
        0x04cU, 0xb84U, 0x538U, 0x43aU, 0x25dU, 0xa4cU
    };
    static const uint32_t value[12] = {
        0xd2daU, 0xfafeU, 0x1b48U, 0x3610U, 0xaf17U, 0xa2f5U,
        0x27a9U, 0xca22U, 0xd8c2U, 0x5890U, 0xda59U, 0x4a40U
    };
    static const unsigned heads[8] = {4, 9, 11, 5, 0, 1, 6, 8};
    unsigned node = heads[challenge & 7U];
    unsigned steps = (unsigned)((challenge >> 3) & 3U);
    uint32_t accumulator = (uint32_t)(challenge >> 6)
                           ^ (uint32_t)(challenge >> 32) ^ 0xa681U;
    for (unsigned i = 0; i <= steps; ++i) {
        if (node >= 12) return 1;
        accumulator = (3U * accumulator + value[node]) ^ id[node];
        values[1] = id[node];
        node = next[node];
    }
    values[0] = accumulator;
    return write_answer(out, cap, values, count);
}

int solve_phase_6(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 1;
    uint32_t x = (uint32_t)(challenge >> 6) ^ (uint32_t)(challenge >> 32);
    uint32_t first = phase6_op((unsigned)(challenge & 7U), x) ^ 0xd181U;
    values[0] = phase6_op((unsigned)((challenge >> 3) & 7U), first) + 0x95afU;
    return write_answer(out, cap, values, count);
}

int solve_secret(uint64_t challenge, uint64_t context, char *out, size_t cap) {
    uint32_t selector = (uint32_t)(context & 3U);
    uint32_t c = (uint32_t)challenge ^ (uint32_t)(challenge >> 32);
    uint32_t t = (uint32_t)context ^ (uint32_t)(context >> 32);
    uint32_t second = rotate_right(c ^ t, 7) ^ 0xbed8U
                      ^ (selector * 0x3314U) ^ (uint32_t)(context >> 16);
    uint32_t third = second ^ (t + selector + 0x8dccU + (c >> 7));
    /* The gate copies into a local aggregate: +32 is the OPEN flag,
     * +36 is the numeric payload. Stay within its checked length.
     */
    int size = snprintf(out, cap,
                        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAOPEN%" PRIu32
                        " %" PRIu32 " %" PRIu32,
                        selector, second, third);
    return size < 0 || (size_t)size >= cap;
}
