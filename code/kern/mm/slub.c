#include <slub.h>
#include <pmm.h>
#include <list.h>
#include <stdio.h>

#define SLAB_MAGIC 0x534c5542534c4142ULL
#define LARGE_MAGIC 0x534c55424c415247ULL

static const size_t size_classes[] = {
    8, 16, 32, 64, 128, 256, 512, 1024, 2048,
};
#define NR_CACHES (sizeof(size_classes) / sizeof(size_classes[0]))

struct slub_cache {
    size_t object_size;
    size_t object_offset;
    size_t objects_per_slab;
    list_entry_t partial;
    size_t nr_slabs;
    size_t nr_inuse;
};

// 元数据只占用已申请页的内容，不借用物理页描述符字段。
struct slab_header {
    uint64_t magic;
    struct slub_cache *cache;
    size_t nr_free;
    void *freelist;
    list_entry_t link;
};

struct large_header {
    uint64_t magic;
    size_t npages;
};

static struct slub_cache caches[NR_CACHES];
static bool initialized;
static size_t live_objects;

void
slub_init(void) {
    // 重置前不得有未归还的对象，否则会丢失所属 slab。
    assert(live_objects == 0);
    for (size_t i = 0; i < NR_CACHES; i ++) {
        struct slub_cache *cache = &caches[i];
        cache->object_size = size_classes[i];
        cache->object_offset = ROUNDUP(sizeof(struct slab_header),
                                       cache->object_size);
        cache->objects_per_slab =
            (PGSIZE - cache->object_offset) / cache->object_size;
        assert(cache->objects_per_slab > 0);
        list_init(&cache->partial);
        cache->nr_slabs = cache->nr_inuse = 0;
    }
    initialized = 1;
}

static struct slab_header *
new_slab(struct slub_cache *cache) {
    struct Page *page = alloc_pages(1);
    if (page == NULL) {
        return NULL;
    }
    struct slab_header *slab =
        (struct slab_header *)(page2pa(page) + va_pa_offset);
    slab->magic = SLAB_MAGIC;
    slab->cache = cache;
    slab->nr_free = cache->objects_per_slab;
    slab->freelist = NULL;
    list_init(&slab->link);

    // 空闲对象的前一个指针大小空间保存下一个空闲对象。
    for (size_t i = cache->objects_per_slab; i > 0; i --) {
        void *obj = (void *)((uintptr_t)slab + cache->object_offset +
                             (i - 1) * cache->object_size);
        *(void **)obj = slab->freelist;
        slab->freelist = obj;
    }
    list_add(&cache->partial, &slab->link);
    cache->nr_slabs ++;
    return slab;
}

static void *
alloc_large(size_t size) {
    // 同时防止加页头和向上页对齐发生无符号回绕。
    if (size > (size_t)-1 - sizeof(struct large_header) - (PGSIZE - 1)) {
        return NULL;
    }
    size_t npages = ROUNDUP(size + sizeof(struct large_header), PGSIZE)
                    / PGSIZE;
    struct Page *page = alloc_pages(npages);
    if (page == NULL) {
        return NULL;
    }
    struct large_header *header =
        (struct large_header *)(page2pa(page) + va_pa_offset);
    header->magic = LARGE_MAGIC;
    header->npages = npages;
    live_objects ++;
    return header + 1;
}

void *
kmalloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    assert(initialized);
    for (size_t i = 0; i < NR_CACHES; i ++) {
        struct slub_cache *cache = &caches[i];
        if (size > cache->object_size) {
            continue;
        }
        struct slab_header *slab;
        if (list_empty(&cache->partial)) {
            slab = new_slab(cache);
            if (slab == NULL) {
                return NULL;
            }
        } else {
            slab = to_struct(list_next(&cache->partial),
                             struct slab_header, link);
        }
        assert(slab->magic == SLAB_MAGIC && slab->cache == cache);
        assert(slab->nr_free > 0 && slab->freelist != NULL);
        void *obj = slab->freelist;
        slab->freelist = *(void **)obj;
        slab->nr_free --;
        cache->nr_inuse ++;
        live_objects ++;
        if (slab->nr_free == 0) {
            list_del_init(&slab->link);
        }
        return obj;
    }
    return alloc_large(size);
}

static void
free_small(struct slab_header *slab, void *ptr) {
    struct slub_cache *cache = NULL;
    for (size_t i = 0; i < NR_CACHES; i ++) {
        if (slab->cache == &caches[i]) {
            cache = &caches[i];
            break;
        }
    }
    if (cache == NULL) {
        panic("kfree: invalid slab cache for %p", ptr);
    }
    uintptr_t first = (uintptr_t)slab + cache->object_offset;
    uintptr_t addr = (uintptr_t)ptr;
    if (addr < first ||
        addr >= first + cache->objects_per_slab * cache->object_size ||
        (addr - first) % cache->object_size != 0) {
        panic("kfree: invalid slab object address %p", ptr);
    }
    // 沿本页的空闲链表检查重复释放，不另占 bitmap 或 Page 字段。
    for (void *obj = slab->freelist; obj != NULL; obj = *(void **)obj) {
        if (obj == ptr) {
            panic("kfree: double free of %p", ptr);
        }
    }
    assert(slab->nr_free < cache->objects_per_slab);
    assert(cache->nr_inuse > 0 && live_objects > 0);
    if (slab->nr_free == 0) {
        list_add(&cache->partial, &slab->link);
    }
    *(void **)ptr = slab->freelist;
    slab->freelist = ptr;
    slab->nr_free ++;
    cache->nr_inuse --;
    live_objects --;
    if (slab->nr_free == cache->objects_per_slab) {
        list_del_init(&slab->link);
        assert(cache->nr_slabs > 0);
        cache->nr_slabs --;
        slab->magic = 0;
        free_pages(pa2page(PADDR(slab)), 1);
    }
}

void
kfree(void *ptr) {
    if (ptr == NULL) {
        return;
    }
    assert(initialized);
    uintptr_t addr = (uintptr_t)ptr;
    if (addr < KERNBASE || addr >= KERNTOP) {
        panic("kfree: invalid kernel address %p", ptr);
    }
    uintptr_t base = ROUNDDOWN(addr, PGSIZE);
    uintptr_t pa = PADDR(base);
    if (PPN(pa) < nbase || PPN(pa) >= npage) {
        panic("kfree: address outside managed memory %p", ptr);
    }
    uint64_t magic = *(uint64_t *)base;
    if (magic == SLAB_MAGIC) {
        free_small((struct slab_header *)base, ptr);
    } else if (magic == LARGE_MAGIC) {
        struct large_header *header = (struct large_header *)base;
        if (ptr != (void *)(header + 1) || header->npages == 0 ||
            header->npages > npage - PPN(pa)) {
            panic("kfree: invalid large object address or header %p", ptr);
        }
        size_t npages = header->npages;
        assert(live_objects > 0);
        live_objects --;
        header->magic = 0;
        free_pages(pa2page(PADDR(header)), npages);
    } else {
        panic("kfree: invalid magic for %p", ptr);
    }
}

// 以下辅助函数仅用于 slub_check，不改变分配器的实现或内部状态。
static void
slub_test_empty(size_t expected_pages) {
    assert(initialized && live_objects == 0);
    assert(nr_free_pages() == expected_pages);
    for (size_t i = 0; i < NR_CACHES; i ++) {
        struct slub_cache *cache = &caches[i];
        assert(cache->nr_slabs == 0 && cache->nr_inuse == 0);
        assert(list_empty(&cache->partial));
        assert(list_prev(&cache->partial) == &cache->partial);
        assert(cache->object_size == size_classes[i]);
        assert(cache->object_offset == ROUNDUP(sizeof(struct slab_header),
                                               cache->object_size));
        assert(cache->objects_per_slab ==
               (PGSIZE - cache->object_offset) / cache->object_size);
    }
}

static void
slub_test_fill(void *ptr, size_t size, uint8_t pattern) {
    volatile uint8_t *bytes = ptr;
    for (size_t i = 0; i < size; i ++) {
        bytes[i] = pattern;
    }
}

static void
slub_test_read(const void *ptr, size_t size, uint8_t pattern) {
    const volatile uint8_t *bytes = ptr;
    for (size_t i = 0; i < size; i ++) {
        assert(bytes[i] == pattern);
    }
}

static struct slab_header *
slub_test_small(void *ptr, size_t size) {
    assert(ptr != NULL && ((uintptr_t)ptr & 7) == 0);
    struct slab_header *slab =
        (struct slab_header *)ROUNDDOWN((uintptr_t)ptr, PGSIZE);
    assert(slab->magic == SLAB_MAGIC);
    size_t class_idx = 0;
    while (class_idx < NR_CACHES && size_classes[class_idx] < size) {
        class_idx ++;
    }
    assert(class_idx < NR_CACHES);
    struct slub_cache *cache = &caches[class_idx];
    assert(slab->cache == cache && cache->object_size == size_classes[class_idx]);
    size_t offset = (uintptr_t)ptr - (uintptr_t)slab;
    assert(offset >= cache->object_offset && offset < PGSIZE);
    assert((offset - cache->object_offset) % cache->object_size == 0);
    assert(size <= cache->object_size && cache->object_size <= PGSIZE - offset);
    return slab;
}

static bool
slub_test_partial_has(struct slub_cache *cache, struct slab_header *slab) {
    list_entry_t *head = &cache->partial;
    size_t visited = 0;
    for (list_entry_t *le = list_next(head); le != head; le = list_next(le)) {
        assert(visited < cache->nr_slabs);
        visited ++;
        assert(list_prev(list_next(le)) == le);
        assert(list_next(list_prev(le)) == le);
        if (le == &slab->link) {
            return 1;
        }
    }
    return 0;
}

static uint32_t
slub_test_random(uint32_t *state) {
    *state = *state * 1664525U + 1013904223U;
    return *state;
}

void
slub_check(void) {
    cprintf("slub_check: zero and null inputs\n");
    size_t initial_pages = nr_free_pages();
    slub_test_empty(initial_pages);
    assert(kmalloc(0) == NULL);
    slub_test_empty(initial_pages);
    kfree(NULL);
    slub_test_empty(initial_pages);

    cprintf("slub_check: size classes and alignment\n");
    const size_t requests[] = {
        1, 8, 9, 16, 17, 31, 32, 33, 64, 100, 128, 200,
        256, 300, 512, 700, 1024, 1500, 2048,
    };
    void *objects[sizeof(requests) / sizeof(requests[0])];
    size_t count = sizeof(requests) / sizeof(requests[0]);
    for (size_t i = 0; i < count; i ++) {
        objects[i] = kmalloc(requests[i]);
        slub_test_small(objects[i], requests[i]);
        assert(live_objects == i + 1);
        for (size_t j = 0; j < i; j ++) {
            uintptr_t a = (uintptr_t)objects[i], b = (uintptr_t)objects[j];
            assert(a + requests[i] <= b || b + requests[j] <= a);
        }
    }

    cprintf("slub_check: independent object data\n");
    for (size_t i = 0; i < count; i ++) {
        slub_test_fill(objects[i], requests[i], (uint8_t)(0x51 + 7 * i));
    }
    for (size_t i = 0; i < count; i ++) {
        slub_test_read(objects[i], requests[i], (uint8_t)(0x51 + 7 * i));
    }
    for (size_t i = 0; i < count; i ++) {
        // 先释放的对象不应破坏仍在使用的相邻对象或同页对象。
        slub_test_read(objects[i], requests[i], (uint8_t)(0x51 + 7 * i));
        kfree(objects[i]);
    }
    slub_test_empty(initial_pages);

    cprintf("slub_check: slab expansion and full-list removal\n");
    size_t class_idx = 0;
    while (class_idx < NR_CACHES && size_classes[class_idx] != 64) {
        class_idx ++;
    }
    assert(class_idx < NR_CACHES);
    struct slub_cache *cache = &caches[class_idx];
    size_t capacity = cache->objects_per_slab;
    size_t old_slabs = cache->nr_slabs, old_inuse = cache->nr_inuse;
    size_t expansion_pages = nr_free_pages();
    void *expanded[PGSIZE / 64 + 1];
    assert(capacity > 1 && capacity + 1 <= sizeof(expanded) / sizeof(expanded[0]));
    struct slab_header *first = NULL;
    for (size_t i = 0; i <= capacity; i ++) {
        expanded[i] = kmalloc(64);
        struct slab_header *slab = slub_test_small(expanded[i], 64);
        if (i == 0) {
            first = slab;
        }
        if (i < capacity) {
            assert(slab == first);
        } else {
            assert(slab != first);
        }
        slub_test_fill(expanded[i], 64, (uint8_t)(0x21 + i));
        if (i + 1 == capacity) {
            assert(first->nr_free == 0 && first->freelist == NULL);
            assert(!slub_test_partial_has(cache, first));
        }
    }
    assert(cache->nr_slabs == old_slabs + 2);
    assert(cache->nr_inuse == old_inuse + capacity + 1);
    assert(nr_free_pages() == expansion_pages - 2);
    assert(!slub_test_partial_has(cache, first));

    cprintf("slub_check: LIFO reuse\n");
    slub_test_read(expanded[0], 64, 0x21);
    kfree(expanded[0]);
    assert(slub_test_partial_has(cache, first));
    assert(cache->nr_slabs == old_slabs + 2);
    void *reused = kmalloc(64);
    assert(reused == expanded[0]);
    expanded[0] = reused;
    slub_test_fill(reused, 64, 0x21);
    assert(!slub_test_partial_has(cache, first));
    assert(cache->nr_inuse == old_inuse + capacity + 1);

    cprintf("slub_check: empty slabs return their pages\n");
    for (size_t i = 0; i <= capacity; i ++) {
        slub_test_read(expanded[i], 64, (uint8_t)(0x21 + i));
        kfree(expanded[i]);
    }
    assert(cache->nr_slabs == old_slabs && cache->nr_inuse == old_inuse);
    assert(nr_free_pages() == expansion_pages);
    slub_test_empty(initial_pages);

    cprintf("slub_check: large objects and page accounting\n");
    const size_t large_sizes[] = {2049, PGSIZE - 16, PGSIZE, 3 * PGSIZE + 1};
    for (size_t i = 0; i < sizeof(large_sizes) / sizeof(large_sizes[0]); i ++) {
        size_t pages_before = nr_free_pages();
        size_t needed = ROUNDUP(large_sizes[i] + sizeof(struct large_header),
                                PGSIZE) / PGSIZE;
        void *ptr = kmalloc(large_sizes[i]);
        assert(ptr != NULL && ((uintptr_t)ptr & 7) == 0);
        assert(nr_free_pages() == pages_before - needed && live_objects == 1);
        struct large_header *header =
            (struct large_header *)ROUNDDOWN((uintptr_t)ptr, PGSIZE);
        assert(header->magic == LARGE_MAGIC && header->npages == needed);
        assert(ptr == (void *)(header + 1));
        slub_test_fill(ptr, large_sizes[i], (uint8_t)(0xa0 + i));
        slub_test_read(ptr, large_sizes[i], (uint8_t)(0xa0 + i));
        kfree(ptr);
        slub_test_empty(initial_pages);
    }

    cprintf("slub_check: deterministic interleaved stress\n");
    void *slots[32] = {NULL};
    size_t sizes[32] = {0};
    uint8_t patterns[32] = {0};
    uint32_t random_state = 0x2412172U;
    size_t active = 0;
    for (size_t step = 0; step < 512; step ++) {
        // 取高位选择槽位，避免线性同余序列低位短周期影响覆盖。
        size_t slot = (slub_test_random(&random_state) >> 16) % 32;
        if (slots[slot] != NULL) {
            slub_test_read(slots[slot], sizes[slot], patterns[slot]);
            kfree(slots[slot]);
            slots[slot] = NULL;
            active --;
        } else {
            sizes[slot] = 1 + (slub_test_random(&random_state) >> 8) % 3000;
            slots[slot] = kmalloc(sizes[slot]);
            assert(slots[slot] != NULL && ((uintptr_t)slots[slot] & 7) == 0);
            for (size_t j = 0; j < 32; j ++) {
                if (j != slot && slots[j] != NULL) {
                    uintptr_t a = (uintptr_t)slots[slot], b = (uintptr_t)slots[j];
                    assert(a + sizes[slot] <= b || b + sizes[j] <= a);
                }
            }
            patterns[slot] = (uint8_t)(step * 37 + slot * 17 + 1);
            slub_test_fill(slots[slot], sizes[slot], patterns[slot]);
            active ++;
        }
        assert(live_objects == active);
    }
    for (size_t i = 0; i < 32; i ++) {
        if (slots[i] != NULL) {
            slub_test_read(slots[i], sizes[i], patterns[i]);
            kfree(slots[i]);
            active --;
        }
    }
    assert(active == 0);
    slub_test_empty(initial_pages);

    cprintf("slub_check: failed allocations preserve state\n");
    size_t failure_pages = nr_free_pages(), failure_live = live_objects;
    assert(kmalloc((size_t)-1) == NULL);
    assert(nr_free_pages() == failure_pages && live_objects == failure_live);
    slub_test_empty(initial_pages);
    assert(kmalloc((failure_pages + 1) * PGSIZE) == NULL);
    assert(nr_free_pages() == failure_pages && live_objects == failure_live);
    slub_test_empty(initial_pages);
    cprintf("slub_check() succeeded!\n");
}
