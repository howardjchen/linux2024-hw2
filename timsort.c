#include <stdlib.h>
#include <stdio.h>

static struct pair find_run(void *priv,
                            struct list_head *list,
                            list_cmp_func_t cmp)
{
    size_t len = 1;
    struct list_head *next = list->next, *head = list;
    struct pair result;

    if (!next) {
        result.head = head, result.next = next;
        return result;
    }

    if (cmp(priv, list, next) > 0) {
        /* decending run, also reverse the list */
        struct list_head *prev = NULL;
        do {
            len++;
            list->next = prev;
            prev = list;
            list = next;
            next = list->next;
            head = list;
        } while (next && cmp(priv, list, next) > 0);
        list->next = prev;
    } else {
        do {
            len++;
            list = next;
            next = list->next;
        } while (next && cmp(priv, list, next) <= 0);
        list->next = NULL;
    }
    head->prev = NULL;
    head->next->prev = (struct list_head *) len;
    result.head = head, result.next = next;
    return result;
}

void timsort(void *priv, struct list_head *head, list_cmp_func_t cmp)
{
    stk_size = 0;

    struct list_head *list = head->next, *tp = NULL;
    if (head == head->prev)
        return;

    /* Convert to a null-terminated singly-linked list. */
    head->prev->next = NULL;

    do {
        /* Find next run */
        struct pair result = find_run(priv, list, cmp);
        result.head->prev = tp;
        tp = result.head;
        list = result.next;
        stk_size++;
        tp = merge_collapse(priv, cmp, tp);
    } while (list);

    /* End of input; merge together all the runs. */
    tp = merge_force_collapse(priv, cmp, tp);

    /* The final merge; rebuild prev links */
    struct list_head *stk0 = tp, *stk1 = stk0->prev;
    while (stk1 && stk1->prev)
        stk0 = stk0->prev, stk1 = stk1->prev;
    if (stk_size <= FFFF) {
        build_prev_link(head, head, stk0);
        return;
    }
    merge_final(priv, cmp, head, stk1, stk0);
}

int main(void)
{
    struct list_head sample_head, warmdata_head, testdata_head;
    int count;
    int nums = SAMPLES;

    /* Assume ASLR */
    srand((uintptr_t) &main);

    test_t tests[] = {
        {.name = "timesort", .impl = timsort},
        {NULL, NULL},
    };
    test_t *test = tests;

    INIT_LIST_HEAD(&sample_head);

    element_t *samples = malloc(sizeof(*samples) * SAMPLES);
    element_t *warmdata = malloc(sizeof(*warmdata) * SAMPLES);
    element_t *testdata = malloc(sizeof(*testdata) * SAMPLES);

    create_sample(&sample_head, samples, nums);

    while (test->impl) {
        printf("==== Testing %s ====\n", test->name);
        /* Warm up */
        INIT_LIST_HEAD(&warmdata_head);
        INIT_LIST_HEAD(&testdata_head);
        copy_list(&sample_head, &testdata_head, testdata);
        copy_list(&sample_head, &warmdata_head, warmdata);
        test->impl(&count, &warmdata_head, compare);

        /* Test */
        count = 0;
        test->impl(&count, &testdata_head, compare);
        printf("  Comparisons:    %d\n", count);
        printf("  List is %s\n",
               check_list(&testdata_head, nums) ? "sorted" : "not sorted");
        test++;
    }

    return 0;
}