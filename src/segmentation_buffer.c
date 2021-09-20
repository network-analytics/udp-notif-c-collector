#include "segmentation_buffer.h"

struct segment_buffer *create_segment_buffer()
{
  struct segment_buffer *res = malloc(sizeof(struct segment_buffer));
  if (res == NULL)
  {
    printf("Malloc failed.");
    return NULL;
  }

  for (int i = 0; i < SIZE_BUF; i++)
  {
    res->hash_array[i] = NULL;
  }
  res->count = 0;
  res->cleanup_start_index = 0;
  res->cleanup = 0;
  return res;
}

char *reassemble_payload(struct message_segment_list_cell *msg_seg_list)
{
  char *complete_msg = (char *)malloc(msg_seg_list->total_payload_byte_size);
  if (complete_msg == NULL)
  {
    printf("Malloc failed reassembling message payload\n");
    return NULL;
  }

  char *msg_tmp = complete_msg;
  struct message_segment_list_cell *temp = msg_seg_list;

  while (temp->next != NULL)
  {
    memcpy(msg_tmp, temp->next->content, temp->next->content_size);
    msg_tmp += temp->next->content_size;
    temp = temp->next;
  }
  return complete_msg;
}

uint32_t hashKey(uint32_t gid, uint32_t mid)
{
  return (gid ^ mid) % SIZE_BUF;
}

/**
 * append linked list of options to head and remove reference of options
 */
int append_options(struct message_segment_list_cell *head, unyte_option_t *options)
{
  uint32_t options_length = 0;
  if (head == NULL) return -1;
  if (options == NULL) return -2;
  
  unyte_option_t *tail = options;
  while(tail->next != NULL)
  {
    options_length += tail->next->length;
    tail = tail->next;
  }
  // Malloc new options linked list
  if (head->options_head == NULL)
  {
    head->options_head = build_message_empty_options();
    if (head->options_head == NULL)
    {
      printf("Malloc failed\n");
      return -3;
    }
    head->options_tail = head->options_head;
  }

  // If has options, save them
  if (options->next != NULL)
  {
    head->options_tail->next = options->next;
    head->options_tail = tail;
    // remove options to be able to free options later
    options->next = NULL;
  }
  head->options_length += options_length;
  return 0;
}

int insert_segment(struct segment_buffer *buf, uint32_t gid, uint32_t mid, uint32_t seqnum, int last, uint32_t payload_size, void *content, unyte_option_t *options)
{
  uint32_t hk = hashKey(gid, mid);
  if (buf->hash_array[hk] == NULL)
  {
    buf->hash_array[hk] = malloc(sizeof(struct collision_list_cell));
    if (buf->hash_array[hk] == NULL)
      return -3;
    buf->hash_array[hk]->next = NULL;
  }

  struct collision_list_cell *head = buf->hash_array[hk];
  struct collision_list_cell *cur = head;
  while (cur->next != NULL && (cur->next->gid != gid || cur->next->mid != mid))
  {
    cur = cur->next;
  }
  if (cur->next == NULL)
  {
    cur->next = malloc(sizeof(struct collision_list_cell));
    if (cur->next == NULL)
      return -3;
    cur->next->gid = gid;
    cur->next->mid = mid;
    cur->next->head = create_message_segment_list(gid, mid);
    if (cur->next->head == NULL)
      return -3;
    buf->count++;
    cur->next->next = NULL;
  }
  return insert_into_msl(cur->next->head, seqnum, last, payload_size, content, options);
}

/**
 * inserts a content in a message segment list, maintaining order among the content based on seqnum
 * head must be the empty header cell of a message_segment_list, cannot be NULL
 * seqnum is the sequence number of that segment
 * last is 1 if the segment is the last one of the content, 0 otherwise
 * content is the value to be added at the corresponding seqnum
 * returns 0 if the content was added and message is not complete yet
 * returns 1 if the content was added and message is complete 
 * (a message was stored with last 1, and the length of the list is equal to the total number of segments)
 * returns -1 if a content was already present for this seqnum
 * returns -2 if a content was already present and message is complete
 * returns -3 if a memory allocation failed
 * returns -4 if options is NULL or head is NULL
 */
int insert_into_msl(struct message_segment_list_cell *head, uint32_t seqnum, int last, uint32_t payload_size, void *content, unyte_option_t *options)
{
  // If the segment is the last, we now know the total size of the message.
  if (last == 1)
    head->total_size = seqnum + 1;
  // Search for insert location
  struct message_segment_list_cell *cur = head;
  while (cur->next != NULL && cur->next->seqnum < seqnum)
    cur = cur->next;

  /** cur->next is the insertion location
    * if cur->next is NULL, the content is being placed at the end of list
    * This segment is not a duplicate, hence the current_size must be increased
    */
  if (cur->next == NULL)
  {
    cur->next = malloc(sizeof(struct message_segment_list_cell));
    if (cur->next == NULL)
      return -3;
    cur->next->seqnum = seqnum;
    cur->next->next = NULL;
    cur->next->content = content;
    cur->next->content_size = payload_size;
    head->current_size++;
    head->total_payload_byte_size += payload_size;
    if (append_options(head, options) < 0)
      return -4;
    // return value based on message completeness
    if (head->total_size > 0 && head->current_size == head->total_size)
      return 1;
    else
      return 0;
  }
  else
  {
    // There is a segment present at cur->next
    // Duplicate insert?
    if (cur->next->seqnum == seqnum)
    {
      // duplicate insert. Return value based on message completeness
      if (head->total_size > 0 && head->current_size == head->total_size)
        return -2;
      else
        return -1;
    }
    else
    {
      // not a duplicate insert. create intermediate cell and make it point to the rest of the list;
      struct message_segment_list_cell *temp = cur->next;
      cur->next = malloc(sizeof(struct message_segment_list_cell));
      if (cur->next == NULL)
        return -3;
      cur->next->seqnum = seqnum;
      cur->next->next = temp;
      cur->next->content = content;
      cur->next->content_size = payload_size;
      head->current_size++;
      head->total_payload_byte_size += payload_size;
      if (append_options(head, options) < 0)
        return -4;
      // return value based on message completeness
      if (head->total_size > 0 && head->current_size == head->total_size)
        return 1;
      else
        return 0;
    }
  }
}

struct message_segment_list_cell *get_segment_list(struct segment_buffer *buf, uint32_t gid, uint32_t mid)
{
  uint32_t hk = hashKey(gid, mid);
  // If there is no message at the request hashvalue, we don't have that segment list
  if (buf->hash_array[hk] == NULL)
  {
    return NULL;
  }

  struct collision_list_cell *cur = buf->hash_array[hk];
  while (cur->next != NULL && (cur->next->gid != gid || cur->next->mid != mid))
  {
    cur = cur->next;
  }
  /*If we walked the collision list and could not find a cell with correct gid and mid, we don't have 
    that semgent list */
  if (cur->next == NULL)
  {
    return NULL;
  }
  else
  {
    /*if we stopped before cur->next is NULL, the next cell has the requested gid and mid value */
    return cur->next->head;
  }
}

void print_segment_list_header(struct message_segment_list_cell *head)
{
  printf("Segment list: gid %d mid %d current size %d total size %d\n", head->gid, head->mid, head->current_size, head->total_size);
}

void print_segment_list_int(struct message_segment_list_cell *head)
{
  printf("Segment list: gid %d mid %d current size %d total size %d\n", head->gid, head->mid, head->current_size, head->total_size);
  struct message_segment_list_cell *cur = head;
  while (cur->next != NULL)
  {
    printf("Value at seqnum %d : %d\n", cur->next->seqnum, *((int *)cur->next->content));
    cur = cur->next;
  }
}

void print_segment_list_string(struct message_segment_list_cell *head)
{
  printf("Segment list: gid %d mid %d current size %d total size %d\n", head->gid, head->mid, head->current_size, head->total_size);
  struct message_segment_list_cell *cur = head;
  while (cur->next != NULL)
  {
    printf("%s\n", ((char *)cur->next->content));
    cur = cur->next;
  }
}

struct message_segment_list_cell *create_message_segment_list(uint32_t gid, uint32_t mid)
{
  struct message_segment_list_cell *res = malloc(sizeof(struct message_segment_list_cell));
  if (res == NULL)
    return NULL;
  res->gid = gid;
  res->mid = mid;
  res->current_size = 0;
  res->total_size = 0;
  res->total_payload_byte_size = 0;
  res->next = NULL;
  res->to_clean_up = 0;
  res->timestamp = 0;
  res->options_head = NULL;
  res->options_tail = NULL;
  res->options_length = 0;
  return res;
}

/**
 * head -> NOTNULL
 * cur : head (->NOTNULL)
 * next: NULL
 * tant que cur->next!=NULL
 *     next 
 */
void clear_msl(struct message_segment_list_cell *head)
{
  struct message_segment_list_cell *cur = head->next;
  struct message_segment_list_cell *temp;
  while (cur != NULL)
  {
    temp = cur->next;
    free(cur->content);
    free(cur);
    cur = temp;
  }
  // freeing options linked list
  unyte_option_t *head_opt = head->options_head;
  if (head_opt != NULL)
  {
    unyte_option_t *cur_opt = head_opt->next;
    unyte_option_t *to_rm;
    while(cur_opt != NULL)
    {
      free(cur_opt->data);
      to_rm = cur_opt;
      cur_opt = cur_opt->next;
      free(to_rm);
    }
    free(head_opt);
  }
  free(head);
}

int clear_collision_list(struct segment_buffer *buf, struct collision_list_cell *head)
{
  int res = 0;
  struct collision_list_cell *cur = head->next;
  struct collision_list_cell *temp;
  while (cur != NULL)
  {
    res++;
    temp = cur->next;
    clear_segment_list(buf, cur->head->gid, cur->head->mid);
    cur = temp;
  }
  free(head);
  return res;
}

int clear_buffer(struct segment_buffer *buf)
{
  int res = 0;
  for (int i = 0; i < SIZE_BUF; i++)
  {
    if (buf->hash_array[i] != NULL)
    {
      //There is a collision list header cell at hasharray[i]
      //The collision list is not empty. Clear it.
      res += clear_collision_list(buf, buf->hash_array[i]);
    }
  }
  free(buf);
  return res;
}

void print_collision_list_int(struct collision_list_cell *cell)
{
  if (cell != NULL)
  {
    struct collision_list_cell *cur = cell;
    while (cur->next != NULL)
    {
      print_segment_list_int(cur->next->head);
      cur = cur->next;
    }
  }
}

int clear_segment_list(struct segment_buffer *buf, uint32_t gid, uint32_t mid)
{
  uint32_t hk = hashKey(gid, mid);
  if (buf->hash_array[hk] == NULL)
  {
    return -1;
  }
  struct collision_list_cell *cur = buf->hash_array[hk];
  struct collision_list_cell *next = cur->next;
  while (next != NULL && ((next->gid != gid) || next->mid != mid))
  {
    next = next->next;
    cur = cur->next;
  }
  if (next == NULL)
    return -1;
  //if next is not NULL, that means we stopped before the end so gid and mid of cur->next are the ones to delete. So we bypass cur->next from cur-> and free the content of cur->next
  cur->next = next->next;
  clear_msl(next->head);
  free(next);
  return 0;
}

void print_segment_buffer_int(struct segment_buffer *buf)
{
  int i = 0;
  for (i = 0; i < SIZE_BUF; i++)
  {
    if (buf->hash_array[i] != NULL)
    {
      printf("Collision list at hk %d:\n", i);
      print_collision_list_int(buf->hash_array[i]);
    }
  }
}

void cleanup_seg_buff(struct segment_buffer *buf, int cleanup_pass_size)
{
  int count = 0;
  // printf("Cleaning up starting on %d | count: %d \n", (buf->cleanup_start_index + count) % SIZE_BUF, buf->count);
  time_t now = time(NULL);
  while (count < cleanup_pass_size)
  {
    int current_index = (buf->cleanup_start_index + count) % SIZE_BUF;
    // printf("CurrentIndex : %d\n", current_index);
    struct collision_list_cell *next = buf->hash_array[current_index];
    if (next != NULL)
    {
      next = next->next;
      while (next != NULL)
      {
        if (next->head->to_clean_up == 0)
        {
          // printf("Message is old (%d|%d)\n", next->gid, next->mid);
          next->head->to_clean_up = 1;
          next = next->next;
        }
        else
        {
          // printf("Message is to be cleaned (%d|%d)\n", next->gid, next->mid);
          struct collision_list_cell *t = next->next;
          if (next->head->timestamp == 0)
          {
            next->head->timestamp = now;
          }
          if ((now - next->head->timestamp) > EXPIRE_MSG)
          {
            // printf("Actually clearing message (%d|%d) %ld\n", next->gid, next->mid, next->head->timestamp);
            clear_segment_list(buf, next->gid, next->mid);
            next = t;
            buf->count--;
          }
          else
          {
            next = next->next;
          }
        }
      }
    }
    count++;
  }

  // reset cleanup
  buf->cleanup = 0;
  buf->cleanup_start_index = (buf->cleanup_start_index + CLEAN_UP_PASS_SIZE) % SIZE_BUF;
}
