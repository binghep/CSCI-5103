struct node;
typedef struct node *NODEPTR;
struct node_int;
typedef struct node_int *NODEPTR_int;

NODEPTR minimum(NODEPTR root);
NODEPTR maximum(NODEPTR root); 
void leftrotate(NODEPTR *treeroot, NODEPTR x);
void rightrotate(NODEPTR *treeroot, NODEPTR y);
void rbinsertfixup(NODEPTR *treeroot, NODEPTR z);
void rbinsertArrTree(NODEPTR *treeroot, NODEPTR Z);
void rbinsertLowHigh(NODEPTR *treeroot, NODEPTR Z);
void rbtransplant(NODEPTR *treeroot, NODEPTR u, NODEPTR v);
void rbdeletefixup(NODEPTR *treeroot, NODEPTR x);
//==================================================
NODEPTR_int search(NODEPTR_int root, int k);
void leftrotate(NODEPTR_int *treeroot, NODEPTR_int x);
void rightrotate(NODEPTR_int *treeroot, NODEPTR_int y);
void rbinsertfixup(NODEPTR_int *treeroot, NODEPTR_int z);
void rbinsert_int(NODEPTR_int *treeroot, NODEPTR_int Z);
//==================================================
int countNode(NODEPTR treeroot);
void rbdelete(NODEPTR *treeroot, NODEPTR Z);
void rbFixNode(NODEPTR *treeroot, NODEPTR Z_);
void inorder(NODEPTR x);
void inorderCount(NODEPTR x);
void inorderBoost(NODEPTR x,int elapsed);


NODEPTR pickProcess();
void SetTimer(int ms);
int countNode(NODEPTR treeroot);
void timer_handler(int sigNo);
