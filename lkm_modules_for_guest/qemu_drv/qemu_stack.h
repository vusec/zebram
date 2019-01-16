
struct node
{
  unsigned long data;
  struct node* next;
};

struct node* push(struct node* head, unsigned long data);
struct node* pop(struct node *head, unsigned long  *element);
int empty(struct node* head);
void display(struct node* head);

