#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "unyte_utils.h"
#include "segmentation_buffer.h"

void test_segment_lists()
{
    //Create list
    uint32_t gid = 1;
    uint32_t mid = 2;
    struct message_segment_list_cell *seglist = create_message_segment_list(gid, mid);
    printf("Seglist @ %p\n", seglist);

    printf("Clearing\n");
    clear_msl(seglist);
    printf("Done %p\n", seglist);

    seglist = create_message_segment_list(gid, mid);

    //Add a new cell that is last in an empty list, seqnum 0
    int cont = 1;
    printf("Inserting into empty list\n");
    int val = insert_into_msl(seglist, 0, 1, 1, &cont);
    printf("Done inserting\n");
    printf("Added a last cell in an empty list, with seqnum set to 0, rtrn value: %d\n", val);
    print_segment_list_int(seglist);
    printf("\n");

    printf("Clearing\n");
    clear_msl(seglist);
    printf("Done %p\n", seglist);

    gid = 3;
    mid = 4;

    struct message_segment_list_cell *seglist2 = create_message_segment_list(gid, mid);
    printf("Seglist @ %p\n", seglist2);

    //Add a new cell that is not last in an empty list, seqnum 0
    int cont2 = 1;
    val = insert_into_msl(seglist2, 0, 0, 1, &cont2);
    printf("Added a non last cell in an empty list, with seqnum set to 0, rtrn value: %d\n", val);
    print_segment_list_int(seglist2);
    int cont3 = 10;
    val = insert_into_msl(seglist2, 0, 0, 1, &cont3);
    print_segment_list_int(seglist2);
    printf("Added a non last cell in non empty list, with seqnum set to 0, duplicate, rtrn value: %d\n", val);
    /* int insert_into_msl(struct message_segment_list_cell* head, uint32_t seqnum, int last, void* content);*/

    int cont4 = 2;
    val = insert_into_msl(seglist2, 1, 0, 1, &cont4);
    print_segment_list_int(seglist2);
    printf("Added a non last cell in non empty list, with seqnum set to 1, not duplicate, rtrn value: %d\n", val);

    int cont5 = 4;
    val = insert_into_msl(seglist2, 3, 0, 1, &cont5);
    print_segment_list_int(seglist2);
    printf("Added a non last cell in non empty list, with seqnum set to 3, not duplicate, rtrn value: %d\n", val);

    int cont6 = 3;
    val = insert_into_msl(seglist2, 2, 0, 1, &cont6);
    print_segment_list_int(seglist2);
    printf("Added a non last cell in non empty list, with seqnum set to 2, not duplicate, rtrn value: %d\n", val);

    int cont7 = 6;
    val = insert_into_msl(seglist2, 5, 1, 1, &cont7);
    print_segment_list_int(seglist2);
    printf("Added a last cell in non empty list, with seqnum set to 4, not duplicate, message not complete, rtrn value: %d\n", val);

    int cont8 = 5;
    val = insert_into_msl(seglist2, 4, 0, 1, &cont8);
    print_segment_list_int(seglist2);
    printf("Added a non last cell in non empty list, with seqnum set to 0, not duplicate, msg complete rtrn value: %d\n", val);

    printf("Clearing\n");
    clear_msl(seglist2);
    printf("Done %p\n", seglist2);

    printf("\n");

    struct message_segment_list_cell *seglist3 = create_message_segment_list(gid, mid);

    printf("Seglist @ %p\n", seglist3);
    int cont10 = 0;
    int cont11 = 1;
    int cont12 = 2;
    val = insert_into_msl(seglist3, 1, 0, 1, &cont11);
    print_segment_list_int(seglist3);
    printf("rtrn: %d\n", val);
    val = insert_into_msl(seglist3, 0, 0, 1, &cont10);
    print_segment_list_int(seglist3);
    printf("rtrn: %d\n", val);
    val = insert_into_msl(seglist3, 2, 1, 1, &cont12);
    print_segment_list_int(seglist3);
    printf("rtrn: %d\n", val);

    printf("Clearing\n");
    clear_msl(seglist3);
    printf("Done %p\n", seglist3);
}

void test_segment_buffers()
{
    int i = 0;
    struct segment_buffer *buf = create_segment_buffer();
    for (i = 0; i < 10; i++)
    {
        int *val = malloc(sizeof(int));
        *val = i + 1;
        insert_segment(buf, 1, 2, i, 0, 1, val);
        val = malloc(sizeof(int));
        *val = i;
        *val = *val * *val;
        insert_segment(buf, 2, 1, i, 0, 1, val);
    }

    print_segment_buffer_int(buf);
    /*int insert_segment(struct segment_buffer* buf, uint32_t gid, uint32_t mid, uint32_t seqnum, int last, void* content);*/
    clear_segment_list(buf, 1, 2);
    printf("Cleared list from 1,2\n");

    clear_segment_list(buf, 2, 1);
    printf("Cleared list from 2,1\n");
    print_segment_buffer_int(buf);

    for (i = 0; i < 10; i++)
    {
        int *val = malloc(sizeof(int));
        *val = i + 10;
        insert_segment(buf, 1, 2, i, 0, 1, val);
        val = malloc(sizeof(int));
        *val = i;
        *val = (*val * *val) * (*val);
        insert_segment(buf, 2, 1, i, 0, 1, val);
    }
    print_segment_buffer_int(buf);

    int resclear = clear_buffer(buf);

    printf("Cleared buffer from %d entries\n", resclear);
}

void test_hashes()
{
    uint32_t coucou1 = 1;
    uint32_t coucou2 = 2;
    uint32_t hashed = hashKey(coucou1, coucou2);
    uint32_t hashed2 = hashKey(coucou2, coucou1);
    printf("Coucou %d %d\n", hashed, hashed2);
}
int main()
{
    test_hashes();
    /*    test_segment_lists();*/
    test_segment_buffers();
    return 0;
}
