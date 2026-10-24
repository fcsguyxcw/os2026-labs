#include <buddy_pmm.h>
#include <list.h>
#include <stdio.h>

// 2^15 个 4KiB 页覆盖 128MiB；对齐和伙伴计算均以管理区域起点为基准。
#define MAX_ORDER 15

static struct {
    list_entry_t free_list;
    size_t nr_blocks;
} free_areas[MAX_ORDER + 1];

static struct Page *region_base;
static size_t region_pages;
static size_t nr_free;

// property 记录阶数：空闲块头置 PG_property，已分配块头清除此位。
// 块内其余页不置 PG_property；order 0 与普通页用块头标志区分。
static unsigned int
buddy_order(size_t n) {
    unsigned int order = 0;
    while (order < MAX_ORDER && ((size_t)1 << order) < n) {
        order ++;
    }
    return ((size_t)1 << order) < n ? MAX_ORDER + 1 : order;
}

static void
buddy_insert(struct Page *base, unsigned int order) {
    assert(order <= MAX_ORDER);
    assert(!PageProperty(base));
    list_entry_t *head = &free_areas[order].free_list;
    list_entry_t *le = list_next(head);
    while (le != head && le2page(le, page_link) < base) {
        le = list_next(le);
    }
    base->property = order;
    SetPageProperty(base);
    list_add_before(le, &base->page_link);
    free_areas[order].nr_blocks ++;
}

static void
buddy_remove(struct Page *base, unsigned int order) {
    assert(order <= MAX_ORDER);
    assert(PageProperty(base) && base->property == order);
    assert(free_areas[order].nr_blocks > 0);
    list_del(&base->page_link);
    free_areas[order].nr_blocks --;
    ClearPageProperty(base);
    base->property = 0;
}

static void
buddy_init(void) {
    for (unsigned int order = 0; order <= MAX_ORDER; order ++) {
        list_init(&free_areas[order].free_list);
        free_areas[order].nr_blocks = 0;
    }
    nr_free = 0;
    region_base = NULL;
    region_pages = 0;
}

static void
buddy_init_memmap(struct Page *base, size_t n) {
    assert(n > 0);
    if (region_base != NULL) {
        panic("buddy_init_memmap: only one memory region is supported");
    }
    for (struct Page *p = base; p != base + n; p ++) {
        assert(PageReserved(p));
        p->flags = p->property = 0;
        set_page_ref(p, 0);
    }
    region_base = base;
    region_pages = n;

    // 每次选取当前位置可容纳且相对下标对齐的最大块，保留所有尾页。
    size_t idx = 0;
    while (idx < n) {
        unsigned int order = MAX_ORDER;
        while (((size_t)1 << order) > n - idx ||
               (idx & (((size_t)1 << order) - 1)) != 0) {
            order --;
        }
        buddy_insert(base + idx, order);
        idx += (size_t)1 << order;
    }
    nr_free = n;
}

static struct Page *
buddy_alloc_pages(size_t n) {
    assert(n > 0);
    if (n > nr_free) {
        return NULL;
    }
    unsigned int order = buddy_order(n);
    if (order > MAX_ORDER) {
        return NULL;
    }
    unsigned int found = order;
    while (found <= MAX_ORDER && list_empty(&free_areas[found].free_list)) {
        found ++;
    }
    if (found > MAX_ORDER) {
        return NULL;
    }

    struct Page *base = le2page(list_next(&free_areas[found].free_list), page_link);
    buddy_remove(base, found);
    while (found > order) {
        found --;
        buddy_insert(base + ((size_t)1 << found), found);
    }
    base->property = order;
    // 向上取整多出的页面属于内部碎片，调用者释放时须随整块一起归还。
    nr_free -= (size_t)1 << order;
    return base;
}

static void
buddy_free_pages(struct Page *base, size_t n) {
    assert(n > 0);
    unsigned int order = buddy_order(n);
    assert(order <= MAX_ORDER);
    assert(region_base != NULL);
    assert(base >= region_base && base < region_base + region_pages);
    assert(!PageProperty(base) && base->property == order);
    size_t idx = base - region_base;
    size_t released = (size_t)1 << order;
    assert((idx & (released - 1)) == 0);
    assert(released <= region_pages - idx);
    for (struct Page *p = base; p != base + released; p ++) {
        assert(!PageReserved(p) && !PageProperty(p));
        p->flags = p->property = 0;
        set_page_ref(p, 0);
    }

    while (order < MAX_ORDER) {
        size_t size = (size_t)1 << order;
        size_t buddy_idx = idx ^ size;
        if (buddy_idx >= region_pages || size > region_pages - buddy_idx) {
            break;
        }
        struct Page *buddy = region_base + buddy_idx;
        if (!PageProperty(buddy) || buddy->property != order) {
            break;
        }
        buddy_remove(buddy, order);
        if (buddy_idx < idx) {
            idx = buddy_idx;
        }
        order ++;
    }
    buddy_insert(region_base + idx, order);
    nr_free += released;
}

static size_t
buddy_nr_free_pages(void) {
    return nr_free;
}

const struct pmm_manager buddy_pmm_manager = {
    .name = "buddy_pmm_manager",
    .init = buddy_init,
    .init_memmap = buddy_init_memmap,
    .alloc_pages = buddy_alloc_pages,
    .free_pages = buddy_free_pages,
    .nr_free_pages = buddy_nr_free_pages,
    .check = buddy_check,
};

// 测试只通过分配/释放接口改变状态；标记数组仅用于检测空闲块重叠。
static void
buddy_test_invariants(void) {
    static unsigned char seen[(size_t)1 << MAX_ORDER];
    assert(region_base != NULL && region_pages > 0);
    assert(region_pages <= ((size_t)1 << MAX_ORDER));
    for (size_t i = 0; i < region_pages; i ++) {
        seen[i] = 0;
    }
    size_t total = 0;
    for (unsigned int order = 0; order <= MAX_ORDER; order ++) {
        list_entry_t *head = &free_areas[order].free_list;
        assert(list_next(list_prev(head)) == head);
        assert(list_prev(list_next(head)) == head);
        size_t count = 0, previous = 0;
        size_t size = (size_t)1 << order;
        for (list_entry_t *le = list_next(head); le != head; le = list_next(le)) {
            assert(count < region_pages);
            assert(list_next(list_prev(le)) == le);
            assert(list_prev(list_next(le)) == le);
            struct Page *p = le2page(le, page_link);
            assert(p >= region_base && p < region_base + region_pages);
            size_t idx = p - region_base;
            assert(PageProperty(p) && p->property == order);
            assert((idx & (size - 1)) == 0);
            assert(size <= region_pages - idx);
            assert(count == 0 || previous < idx);
            previous = idx;
            for (size_t i = 0; i < size; i ++) {
                assert(!seen[idx + i]);
                seen[idx + i] = 1;
                assert(!PageReserved(p + i) && page_ref(p + i) == 0);
                assert(i == 0 || !PageProperty(p + i));
            }
            count ++;
        }
        assert(count == free_areas[order].nr_blocks);
        total += count * size;
    }
    assert(total == nr_free);
    assert(total == buddy_pmm_manager.nr_free_pages());
}

static struct Page *
buddy_test_alloc(size_t n, unsigned int order) {
    size_t before = nr_free;
    size_t size = (size_t)1 << order;
    struct Page *p = buddy_pmm_manager.alloc_pages(n);
    assert(p != NULL);
    assert(p >= region_base && p < region_base + region_pages);
    size_t idx = p - region_base;
    assert((idx & (size - 1)) == 0 && size <= region_pages - idx);
    assert(!PageProperty(p) && p->property == order);
    assert(before >= size && nr_free == before - size);
    for (size_t i = 0; i < size; i ++) {
        assert(!PageReserved(p + i) && !PageProperty(p + i));
        assert(page_ref(p + i) == 0);
    }
    return p;
}

// 检查隔离出来的大块是否完整，其他初始块此时均被测试借走。
static void
buddy_test_only_block(struct Page *base, unsigned int order) {
    assert(nr_free == ((size_t)1 << order));
    for (unsigned int k = 0; k <= MAX_ORDER; k ++) {
        assert(free_areas[k].nr_blocks == (k == order ? 1 : 0));
        if (k == order) {
            list_entry_t *head = &free_areas[k].free_list;
            assert(list_next(head) == &base->page_link);
            assert(list_prev(head) == &base->page_link);
            assert(PageProperty(base) && base->property == order);
        } else {
            assert(list_empty(&free_areas[k].free_list));
        }
    }
    buddy_test_invariants();
}

void
buddy_check(void) {
    cprintf("buddy_check: initial state and invariants\n");
    assert(nr_free == region_pages && region_pages >= 21);
    buddy_test_invariants();
    size_t initial_free = nr_free;
    size_t initial_counts[MAX_ORDER + 1];
    struct Page *initial_heads[MAX_ORDER + 1];
    struct Page *borrowed[MAX_ORDER + 1];
    unsigned int borrowed_orders[MAX_ORDER + 1];
    size_t borrowed_count = 0;
    unsigned int largest = 0;
    for (unsigned int k = 0; k <= MAX_ORDER; k ++) {
        initial_counts[k] = free_areas[k].nr_blocks;
        // 单区域从低到高贪心切分，且不超过 2^MAX_ORDER 页：每阶至多一块。
        assert(initial_counts[k] <= 1);
        initial_heads[k] = initial_counts[k] == 0 ? NULL :
            le2page(list_next(&free_areas[k].free_list), page_link);
        if (initial_counts[k] != 0) {
            largest = k;
        }
    }
    assert(largest >= 3);

    cprintf("buddy_check: rounding and internal fragmentation\n");
    const size_t requests[] = {3, 1, 5, 8};
    const unsigned int orders[] = {2, 0, 3, 3};
    struct Page *rounded[4];
    for (size_t i = 0; i < 4; i ++) {
        rounded[i] = buddy_test_alloc(requests[i], orders[i]);
        for (size_t j = 0; j < i; j ++) {
            assert(rounded[i] + ((size_t)1 << orders[i]) <= rounded[j] ||
                   rounded[j] + ((size_t)1 << orders[j]) <= rounded[i]);
        }
    }
    assert(nr_free == initial_free - 21);
    buddy_test_invariants();
    for (size_t i = 0; i < 4; i ++) {
        // 让释放测试同时验证整块引用计数清零，包括取整得到的额外页面。
        for (size_t j = 0; j < ((size_t)1 << orders[i]); j ++) {
            set_page_ref(rounded[i] + j, 1);
        }
        buddy_pmm_manager.free_pages(rounded[i], requests[i]);
    }
    assert(nr_free == initial_free);
    buddy_test_invariants();

    cprintf("buddy_check: borrowing initial blocks\n");
    for (unsigned int k = 0; k <= MAX_ORDER; k ++) {
        assert(free_areas[k].nr_blocks == initial_counts[k]);
        if (initial_counts[k] != 0) {
            struct Page *p = buddy_test_alloc((size_t)1 << k, k);
            assert(p == initial_heads[k]);
            assert(borrowed_count < MAX_ORDER + 1);
            borrowed[borrowed_count] = p;
            borrowed_orders[borrowed_count] = k;
            borrowed_count ++;
        }
    }
    assert(nr_free == 0);
    buddy_test_invariants();
    assert(borrowed_count > 0);
    struct Page *isolated = borrowed[borrowed_count - 1];
    assert(borrowed_orders[borrowed_count - 1] == largest);
    buddy_pmm_manager.free_pages(isolated, (size_t)1 << largest);
    buddy_test_only_block(isolated, largest);

    cprintf("buddy_check: splitting at every level\n");
    struct Page *first = buddy_test_alloc(1, 0);
    assert(first == isolated);
    size_t first_idx = first - region_base;
    for (unsigned int k = 0; k <= MAX_ORDER; k ++) {
        assert(free_areas[k].nr_blocks == (k < largest ? 1 : 0));
        if (k < largest) {
            struct Page *partner = region_base + (first_idx ^ ((size_t)1 << k));
            assert(PageProperty(partner) && partner->property == k);
            assert(list_next(&free_areas[k].free_list) == &partner->page_link);
        }
    }
    buddy_test_invariants();

    cprintf("buddy_check: buddy merging\n");
    struct Page *second = buddy_test_alloc(1, 0);
    assert(second == first + 1);
    assert(((size_t)(first - region_base) ^ 1) == (size_t)(second - region_base));
    buddy_pmm_manager.free_pages(first, 1);
    assert(PageProperty(first) && first->property == 0);
    assert(free_areas[0].nr_blocks == 1);
    buddy_test_invariants();
    buddy_pmm_manager.free_pages(second, 1);
    assert(!PageProperty(second));
    assert(free_areas[0].nr_blocks == 0);
    buddy_test_only_block(isolated, largest);

    cprintf("buddy_check: adjacent non-buddies stay separate\n");
    struct Page *adjacent[4];
    for (size_t i = 0; i < 4; i ++) {
        adjacent[i] = buddy_test_alloc(1, 0);
        assert(adjacent[i] == isolated + i);
    }
    size_t left_idx = adjacent[1] - region_base;
    size_t right_idx = adjacent[2] - region_base;
    assert(right_idx == left_idx + 1 && (left_idx ^ 1) != right_idx);
    buddy_pmm_manager.free_pages(adjacent[1], 1);
    buddy_pmm_manager.free_pages(adjacent[2], 1);
    assert(PageProperty(adjacent[1]) && adjacent[1]->property == 0);
    assert(PageProperty(adjacent[2]) && adjacent[2]->property == 0);
    assert(free_areas[0].nr_blocks == 2);
    assert(list_next(&free_areas[0].free_list) == &adjacent[1]->page_link);
    assert(list_next(&adjacent[1]->page_link) == &adjacent[2]->page_link);
    buddy_test_invariants();
    buddy_pmm_manager.free_pages(adjacent[0], 1);
    buddy_pmm_manager.free_pages(adjacent[3], 1);
    buddy_test_only_block(isolated, largest);

    cprintf("buddy_check: allocation failure preserves state\n");
    assert(buddy_pmm_manager.alloc_pages(nr_free + 1) == NULL);
    buddy_test_only_block(isolated, largest);
    assert(buddy_pmm_manager.alloc_pages(((size_t)1 << MAX_ORDER) + 1) == NULL);
    buddy_test_only_block(isolated, largest);
    struct Page *whole = buddy_test_alloc((size_t)1 << largest, largest);
    assert(whole == isolated && nr_free == 0);
    assert(buddy_pmm_manager.alloc_pages(1) == NULL);
    buddy_test_invariants();

    cprintf("buddy_check: restoring initial state\n");
    // isolated 已重新整块分配，现有借块均与最初记录一一对应。
    for (size_t i = 0; i < borrowed_count; i ++) {
        buddy_pmm_manager.free_pages(borrowed[i], (size_t)1 << borrowed_orders[i]);
    }
    assert(nr_free == initial_free);
    for (unsigned int k = 0; k <= MAX_ORDER; k ++) {
        assert(free_areas[k].nr_blocks == initial_counts[k]);
        if (initial_counts[k] != 0) {
            list_entry_t *head = &free_areas[k].free_list;
            assert(list_next(head) == &initial_heads[k]->page_link);
            assert(list_prev(head) == &initial_heads[k]->page_link);
        }
    }
    buddy_test_invariants();
    cprintf("buddy_check() succeeded!\n");
}
