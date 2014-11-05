/* Group member:
 * Jiayi Peng, Yi Li
 * ID: 4645102, 3421834
 * x500: pengx077, lixxx503
 * 
 * 
 * Framework by others:
 * 1. red black tree.
 * 		author: Prakhar Jain.
 * 		related functions: functions start with rb, maximum, minimum, 
 * 		search, leftrotate, rightrotate, inorder. Some of the functons 
 * 		are modified to fit this project. Originaly, the user can find
 * 		a node with its key only. Now, the functions are passed a NODEPTR
 * 		to do stuff(i.e. there is no need to search). 
 * 2. memory manager.
 * 		author: Nicholas Padilla,Nha Nguyen,Jiayi Peng
 * 		This is used to decrease overall running time by allocating a 
 * 		pool of memory chuncks. If we don't use this, every time we 
 * 		process one line of input, we need to call melloc. However, 
 * 		we are allocating for every line of the input file beforehand,
 * 		so if the file can be sanitized greatly, there is a large amount
 * 		of memory wasted(and probably fail to allocate memory if the file
 * 		contains too many lines).
 */



#define DEBUG


#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <string.h>  //memset()
using namespace std;
#include <queue>

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>//sleep(1)

#include <cstdio>
#include <ctime>

extern "C" {
	#include "mm.h"
}
extern "C" {
	int mm_init(mm_t *mm, int num_chunks, int chunk_size);
	void *mm_get(mm_t *mm);
	void mm_put(mm_t *mm, void *chunk);
	void mm_release(mm_t *mm);
	double comp_time(struct timeval time_s, struct timeval time_e);
}

#include "HybridScheduler.h"

#define INTERVAL 2000   //microseconds
//#define TIME_QUANTUM 2  //milliseconds
#define RED		1
#define BLACK	2


int TIME_QUANTUM, aHundredClockTicks;//ms
bool realTime=false; //get the gant chart in real time or in a much faster way.
int ArrTreeLength,LowPriTreeLength=0,HighPriTreeLength=0;
int count=0;//inordercount()
ofstream out;
bool useTimer=false;

mm_t mm,mm_int;

struct node {
	int Pri, DyPri, Pid,Burst,RemainBurst,Arrival,FinishTime; 
	int LastTimeKickedOut;//kicked out by the CPU
	struct node *left, *right, *p;
	int color;
};
 
typedef struct node *NODEPTR;
struct node NIL;
NODEPTR NILPTR = &NIL;
 

struct itimerval timer,timer2;


NODEPTR minimum(NODEPTR root) {
	if (root==NILPTR) {cout<<"no minimum. empty rbtree"<<endl;}
	while (root->left != NILPTR)
		root = root->left;
	return root;
}
 
NODEPTR maximum(NODEPTR root) {
	if (root==NILPTR) {cout<<"no minimum. empty rbtree"<<endl;}
	while (root->right != NILPTR)
		root = root->right;
	return root;
}
 

void leftrotate(NODEPTR *treeroot, NODEPTR x) {
	NODEPTR y = x->right;
	x->right = y->left;
	if (y->left != NILPTR)
		y->left->p = x;
	y->p = x->p;
	if (x->p == NILPTR)
		*treeroot = y;
	else if (x->p->left == x)
		x->p->left = y;
	else
		x->p->right = y;
	y->left = x;
	x->p = y;
}
 
void rightrotate(NODEPTR *treeroot, NODEPTR y) {
	NODEPTR x = y->left;
	y->left = x->right;
	if (x->right != NILPTR)
		x->right->p = y;
	x->p = y->p;
	if (y->p == NILPTR)
		*treeroot = x;
	else if (y->p->left == y)
		y->p->left = x;
	else
		y->p->right = x;
	x->right = y;
	y->p = x;
}
//fix color and balance the red black tree.
void rbinsertfixup(NODEPTR *treeroot, NODEPTR z) {
	while (z->p->color == RED) {
		if (z->p == z->p->p->left) {
			NODEPTR y = z->p->p->right;
			if (y->color == RED) {
				z->p->color = BLACK;
				y->color = BLACK;
				z->p->p->color = RED;
				z = z->p->p;
			}
			else {
				if (z == z->p->right) {
					z = z->p;
					leftrotate(treeroot,z);
				}
				z->p->color = BLACK;
				z->p->p->color = RED;
				rightrotate(treeroot,z->p->p);
			}
		}
		else {
			NODEPTR y = z->p->p->left;
			if (y->color == RED) {
				z->p->color = BLACK;
				y->color = BLACK;
				z->p->p->color = RED;
				z = z->p->p;
			}
			else {
				if (z == z->p->left) {
					z = z->p;
					rightrotate(treeroot,z);
				}
				z->p->color = BLACK;
				z->p->p->color = RED;
				leftrotate(treeroot,z->p->p);
			}
		}
	}
	(*treeroot)->color = BLACK;
}
//insert Z into ArrTree(according to Arrival).
void rbinsertArrTree(NODEPTR *treeroot, NODEPTR Z) { 
	//NODEPTR Z = (NODEPTR) malloc(sizeof(struct node));

//is phase I: building Arrival tree to sort the information according to Arrival time.
	NODEPTR y = NILPTR;
	NODEPTR x = *treeroot;

	while (x != NILPTR) {
		y = x;
		if (Z->Arrival < x->Arrival)
			x = x->left;
		else
			x = x->right;
	}
	Z->p = y;
	if (y == NILPTR)
		*treeroot = Z;
	else if (Z->Arrival < y->Arrival)
		y->left = Z;
	else
		y->right = Z;
		
	Z->left = NILPTR;
	Z->right = NILPTR;
	Z->color = RED;
	rbinsertfixup(treeroot,Z);
}


//connect Z to Low or High Tree(according to Dypri) by modifing its and its neighbours' p, left, right
void rbinsertLowHigh(NODEPTR *treeroot, NODEPTR Z) { 
	//NODEPTR Z = (NODEPTR) malloc(sizeof(struct node));
	

//is phase II: clock tick starts. 
	NODEPTR y = NILPTR;
	NODEPTR x = *treeroot;

	while (x != NILPTR) {
		y = x;
		if (Z->DyPri < x->DyPri)
			x = x->left;
		else
			x = x->right;
	}
	Z->p = y;
	if (y == NILPTR)
		*treeroot = Z;
	else if (Z->DyPri < y->DyPri)
		y->left = Z;
	else
		y->right = Z;
	
	Z->left = NILPTR;
	Z->right = NILPTR;
	Z->color = RED;
	rbinsertfixup(treeroot,Z);
}
 
void rbtransplant(NODEPTR *treeroot, NODEPTR u, NODEPTR v) {
	if (u->p == NILPTR){
		
		*treeroot = v;
	}
	else if (u == u->p->left)
		u->p->left = v;
	else{
		u->p->right = v;
	}
	v->p = u->p;
}
//fix color and balance the red black tree.
void rbdeletefixup(NODEPTR *treeroot, NODEPTR x) {
	while (x != *treeroot && x->color == BLACK) {
		if (x == x->p->left) {
			NODEPTR w = x->p->right;
			if (w->color == RED) {
				w->color = BLACK;
				x->p->color = RED;
				leftrotate(treeroot,x->p);
				w = x->p->right;
			}
			if (w->left->color == BLACK && w->right->color == BLACK) {
				w->color = RED;
				x = x->p;
			}
			else {
			 	if (w->right->color == BLACK) {
					w->left->color = BLACK;
					w->color = RED;
					rightrotate(treeroot,w);
					w = x->p->right;
				}
				w->color = x->p->color;
				x->p->color = BLACK;
				w->right->color = BLACK;
				leftrotate(treeroot,x->p);
				x = *treeroot;
			}
		}
		else {
			NODEPTR w = x->p->left;
			if (w->color == RED) {
				w->color = BLACK;
				x->p->color = RED;
				rightrotate(treeroot,x->p);
				w = x->p->left;
			}
			if (w->left->color == BLACK && w->right->color == BLACK) {
				w->color = RED;
				x = x->p;
			}
			else {
				if (w->left->color == BLACK) {
					w->right->color = BLACK;
					w->color = RED;
					leftrotate(treeroot,w);
					w = x->p->left;
				}
				w->color = x->p->color;
				x->p->color = BLACK;
				w->left->color = BLACK;
				rightrotate(treeroot,x->p);
				x = *treeroot;
			}
		}
	}
	x->color = BLACK;
}


 //==================================int version of rb==================================
struct node_int{
	int key;//Pid
	struct node_int *left,*right,*p;
	int color;
};
typedef struct node_int *NODEPTR_int;
struct node_int NIL_int;
NODEPTR_int NILPTR_int=&NIL_int;

NODEPTR_int search(NODEPTR_int root, int k) {
	if (root == NILPTR_int || root->key == k)
		return root;
	if (k < root->key)
		return search(root->left, k);
	else
		return search(root->right, k);
}


void leftrotate(NODEPTR_int *treeroot, NODEPTR_int x) {
	NODEPTR_int y = x->right;
	x->right = y->left;
	if (y->left != NILPTR_int)
		y->left->p = x;
	y->p = x->p;
	if (x->p == NILPTR_int)
		*treeroot = y;
	else if (x->p->left == x)
		x->p->left = y;
	else
		x->p->right = y;
	y->left = x;
	x->p = y;
}
 
void rightrotate(NODEPTR_int *treeroot, NODEPTR_int y) {
	NODEPTR_int x = y->left;
	y->left = x->right;
	if (x->right != NILPTR_int)
		x->right->p = y;
	x->p = y->p;
	if (y->p == NILPTR_int)
		*treeroot = x;
	else if (y->p->left == y)
		y->p->left = x;
	else
		y->p->right = x;
	x->right = y;
	y->p = x;
}
//fix color and balance the red black tree.
void rbinsertfixup(NODEPTR_int *treeroot, NODEPTR_int z) {
	while (z->p->color == RED) {
		if (z->p == z->p->p->left) {
			NODEPTR_int y = z->p->p->right;
			if (y->color == RED) {
				z->p->color = BLACK;
				y->color = BLACK;
				z->p->p->color = RED;
				z = z->p->p;
			}
			else {
				if (z == z->p->right) {
					z = z->p;
					leftrotate(treeroot,z);
				}
				z->p->color = BLACK;
				z->p->p->color = RED;
				rightrotate(treeroot,z->p->p);
			}
		}
		else {
			NODEPTR_int y = z->p->p->left;
			if (y->color == RED) {
				z->p->color = BLACK;
				y->color = BLACK;
				z->p->p->color = RED;
				z = z->p->p;
			}
			else {
				if (z == z->p->left) {
					z = z->p;
					rightrotate(treeroot,z);
				}
				z->p->color = BLACK;
				z->p->p->color = RED;
				leftrotate(treeroot,z->p->p);
			}
		}
	}
	(*treeroot)->color = BLACK;
}

void rbinsert_int(NODEPTR_int *treeroot, NODEPTR_int Z) {
	//NODEPTR_int Z = (NODEPTR_int) malloc(sizeof(struct node_int));

	NODEPTR_int y = NILPTR_int;
	NODEPTR_int x = *treeroot;
	while (x != NILPTR_int) {
		y = x;
		if (Z->key < x->key)
			x = x->left;
		else
			x = x->right;
	}
	Z->p = y;
	if (y == NILPTR_int)
		*treeroot = Z;
	else if (Z->key < y->key)
		y->left = Z;
	else
		y->right = Z;
	Z->left = NILPTR_int;
	Z->right = NILPTR_int;
	Z->color = RED;
	rbinsertfixup(treeroot,Z);//fix color only
}

	NODEPTR ArrTree = NILPTR;
	NODEPTR LowPriTree = NILPTR;
	NODEPTR HighPriTree = NILPTR;
	NODEPTR_int PIDs=NILPTR_int;//Every process has unique PID.
	queue<NODEPTR> finished;//for statistics only
//=====================================================================================
int countNode(NODEPTR treeroot){
	count=0;
	inorderCount(treeroot);
	return count;
}
	
//change p, left, right of Z's neighbour to make nobody attach to Z, also fix tree color
void rbdelete(NODEPTR *treeroot, NODEPTR Z) {
//	NODEPTR Z = search(*treeroot, z);
	if (Z == NILPTR) {
		printf("Node to be deleted not found\n");
		return;
	}
	void *mm_get(mm_t *mm);
	void mm_put(mm_t *mm, void *chunk);
	NODEPTR y = Z;
	int yoc = y->color;
	NODEPTR x;
	if (Z->left == NILPTR) {
		x = Z->right;
		rbtransplant(treeroot,Z,Z->right);
	}
	else if (Z->right == NILPTR) {

		x = Z->left;
		rbtransplant(treeroot,Z,Z->left);
	}
	else {

		y = minimum(Z->right);
		yoc = y->color;
		x = y->right;
		if (y->p == Z)
			x->p = y;
		else {
			rbtransplant(treeroot,y,y->right);
			y->right = Z->right;
			y->right->p = y;
		}
		rbtransplant(treeroot,Z,y);
		y->left = Z->left;
		y->left->p = y;
		y->color = Z->color;
	}
	if (yoc == BLACK)
		rbdeletefixup(treeroot,x);
}



//DyPri of node has changed, reposition the node in the tree.
void rbFixNode(NODEPTR *treeroot, NODEPTR Z_){

	NODEPTR Z=Z_;
	//delete the node and reinsert according to DyPri
	rbdelete(treeroot, Z);
	//the following is the second part of rbinsert();
	NODEPTR y = NILPTR;
	NODEPTR x = *treeroot;
	
	while (x != NILPTR) {
		y = x;
		if (Z->DyPri < x->DyPri)
			x = x->left;
		else
			x = x->right;
	}
	Z->p = y;
	if (y == NILPTR)
		*treeroot = Z;
	else if (Z->DyPri < y->DyPri)
		y->left = Z;
	else
		y->right = Z;

	Z->left = NILPTR;
	Z->right = NILPTR;
	Z->color = RED;
	rbinsertfixup(treeroot,Z);
}


//tranverse red black tree and print out info according to tree's key in increasing order. it takes treeroot as argument.
//void inorder(NODEPTR x) {
	//if (arrivalQueue){
		//if (x != NILPTR) {
			//inorder(x->left);
			//cout<< "Arrival: "<< x->Arrival << "	Pid: "<<  x->Pid << "	Burst: " << x->Burst << 
					//"	Pri: " << x->Pri <<endl;
			//inorder(x->right);
		//}
	//}else{
		//if (x != NILPTR) {
			//inorder(x->left);
			//cout << "Pri: " << x->Pri <<"	Pid: "<<  x->Pid << "	Burst: " << x->Burst << endl;
			//inorder(x->right);
		//}
	//}
//}


//don't call this. call countNode instead. that returns int.
void inorderCount(NODEPTR x) {
	if (x != NILPTR) {
		inorderCount(x->left);
		//cout<< "Arrival: "<< x->Arrival << "	Pid: "<<  x->Pid << "	Burst: " << x->Burst << 
		//	"	Pri: " << x->Pri <<endl;
		count++;
		inorderCount(x->right);
	}
}


	


//called roughly every 100 ms. x is LowPri first time called. 
//sweep the low pri queue and boost starved processes.

void inorderBoost(NODEPTR x,int elapsed,int times){
		if (x != NILPTR &&LowPriTreeLength!=0) {
			inorderBoost(x->left,elapsed, times);
			//never saw the CPU || haven't seen CPU for aHundredClockTicks
			if (x->LastTimeKickedOut=-1 || elapsed-(x->LastTimeKickedOut) >= aHundredClockTicks){
				x->DyPri+=10*times;
				if (x->DyPri>=50) {x->DyPri=49;}
				
				out<<"Pid "<<x->Pid<<"'s DyPri boosted"<<endl;
				
				rbFixNode(&LowPriTree, x);
			}
			
			inorderBoost(x->right,elapsed,times);
		}
}

////delete all nodes in PIDs red black tree
//void inorderDelete(NODEPTR_int x){
		//if (x != NILPTR_int) {
			//inorderDelete(x->left);
			//NODEPTR_int x_right=x->right;
			//free(x);
			//inorderDelete(x_right);
		//}
//}










	

int elapsed=0;//ms

NODEPTR Running=NILPTR;

//pick highest dyPri process
NODEPTR pickProcess(){
    if (HighPriTreeLength!=0){
	    return maximum(HighPriTree);
    }else if(LowPriTreeLength!=0){
	    return maximum(LowPriTree);
    }else{
	    //both queue are empty
 	    return NILPTR;
    }
}

void SetTimer(int ms){//ms is in millisecond

//  if(realTime){timer.it_value.tv_usec = ms*1000;//microseconds
  
//  }else {//faster mode
//	  timer.it_value.tv_usec=1;
//  } 
	setitimer(ITIMER_VIRTUAL, &timer,NULL); /* Start a virtual timer.*/
}

//void timer_handler2(int sigNo){
	//inorderBoost(LowPriTree,elapsed);
//}

int lastTimeBoost=0,lastTimeFlush;

int HowManyTimes,temp;

void timer_handler(int sigNo)
{
	//sweep LowPriQueue "HowManyTimes"
	if(elapsed-lastTimeBoost>=aHundredClockTicks){
		//accumulated how may times of calling inorderBoost
		HowManyTimes=(elapsed-lastTimeBoost)/aHundredClockTicks;//quotient . min:1
		out<<"...........Sweeping Low Priority queue.............\n";
		inorderBoost(LowPriTree,elapsed,HowManyTimes);
		out<<".................Sweeping Finished...................\n";
		lastTimeBoost+=aHundredClockTicks*HowManyTimes;
		
		#ifdef DEBUG
		temp++;
		//flush every 8 times we sweep
		if(temp%8==0){
			out.flush();
			cout<<"flushing...# of processes done: "<<finished.size()<<endl;
			
		}
		#endif
	}
	
  /*fill the current low, high band queue according to Arrival, which is the key of ArrTree */
  while (ArrTree!=NILPTR && elapsed>= (minimum(ArrTree)->Arrival)){
	    	  
		NODEPTR nextArrival=minimum(ArrTree);
			
		out<<"PID "<< nextArrival->Pid<<" arrives at "<< nextArrival->Arrival<<
		    " ms. Base Priority: "<< nextArrival->Pri <<", Burst: " << nextArrival->Burst <<"\n";
		
		
		//if is user process, connect it to LowPriTree by changing p and its neighbours' p, left, right
		if (nextArrival->Pri<50){
			rbdelete(&ArrTree,nextArrival);//nextArrival losts its p: Z->p=NILPTR
			rbinsertLowHigh(&LowPriTree,nextArrival);//nextArrival's p,left, and right changed.
			//the previous order can't change, otherwise in transplant, Z->p==NILPTR, so ArrTree become empty.
			LowPriTreeLength++;
		//if is high band process, connect it to highPriTree
		}else{
			rbdelete(&ArrTree,nextArrival);
			rbinsertLowHigh(&HighPriTree,nextArrival);
			HighPriTreeLength++;
		}

		ArrTreeLength--;
		if(ArrTreeLength==0) ArrTree=NILPTR;
		//last node in the ArrTree, make it NILPTR (nextArrival is already connected to High or Low queue)
		//if(maximum(ArrTree)==nextArrival) {
			//ArrTree=NILPTR;
			//ArrTreeLength=0;
			//cout<<ArrTreeLength<<"========="<<endl;//0
			//cout<<"LowPriTreeLength: "<<LowPriTreeLength<<", HighPriTreeLength:"<<HighPriTreeLength<<endl;
		//}
		 
		//not last node,do nothing
		//cout<<countNode(ArrTree)<<" in handler 3"<<endl;
  }
  
	 

  //time elapsed since the clock starts.  
  //cout<<elapsed<<" ms: "<<endl;
	
  /*run process with the highest DyPri (maxPri) at this clock tick. */ 
  NODEPTR maxPri=pickProcess();
  //there is nothing in both queues, jump to when next process arrives.
  if (maxPri==NILPTR) {
	  if (ArrTreeLength==0){return;}//if nothing is gonna arrive in the end, set no timer. main while loop exits.
	  

	  elapsed=minimum(ArrTree)->Arrival;
	 if(useTimer){SetTimer(minimum(ArrTree)->Arrival-elapsed);}
	  return;
  }
  //we have something in queues to run: 
  //if something was running and can't finish before the handler is called.
  if (Running!=NILPTR){
	  //if is not the same process as last one, output "... is kicked out..."
	  if(Running->LastTimeKickedOut!=maxPri->LastTimeKickedOut){
		
		out<<"(1) "<<elapsed<<" ms: \n"<<
		"(2) PID "<<Running->Pid<<"'s Quantum expires, remaining burst "<<Running->RemainBurst<<
		"\n(3) PID "<<maxPri->Pid<<" runs, remaining burst "<<maxPri->RemainBurst  <<"\n";
		
	  }
	  //else: if is the same process, output nothing.
  //if nothing is running right prior to this ms.
  }else{
	  out<<"PID "<<maxPri->Pid<<" runs at "<<elapsed<<"ms, remaining burst "<<maxPri->RemainBurst<<"\n";
  }
  //will not finish during this TIME_QUANTUM
  if(maxPri->RemainBurst>TIME_QUANTUM){

		elapsed+=TIME_QUANTUM;
		maxPri->LastTimeKickedOut=elapsed;
		//update RemainBurst
		maxPri->RemainBurst-=TIME_QUANTUM;
	  
		//decrease DyPri by the amount of clock ticks it spent in the CPU prior to the interrupt
		maxPri->DyPri-=TIME_QUANTUM;
		//process priority cannot go below its original base priority
		if(maxPri->DyPri<maxPri->Pri){
			maxPri->DyPri=maxPri->Pri;
		}
		//and is placed in its priority queue
		if (maxPri->Pri<50){
			rbFixNode(&LowPriTree,maxPri);
		}else{
			rbFixNode(&HighPriTree,maxPri);
		}
	  
		//=========save process pointer as "Running". only usage is to output kicked out PID
		Running=maxPri;
	  
		//set up timer. call the handler after TIME_QUANTUM ms.
		SetTimer(TIME_QUANTUM);
		return;
  //will finish beforehand
  }else{
		maxPri->FinishTime=elapsed+maxPri->RemainBurst;
		out<<"PID "<<maxPri->Pid<<" finishes at "<<maxPri->FinishTime<<"ms"<<"\n";
		//update time to when next time the handler is called
		elapsed+=maxPri->RemainBurst;
		maxPri->RemainBurst=0;
		
		//detach the node for statistics. free after outputing statistics
		finished.push(maxPri);
		if((maxPri->Pri)<50){
			LowPriTreeLength--;
			rbdelete(&LowPriTree,maxPri);
		}else{
			HighPriTreeLength--;
			rbdelete(&HighPriTree,maxPri);
		}
		//=========update process pointer "Running"
		Running=NILPTR;
		//set up timer.
		SetTimer(1);
		return;
  }

}
//delete all the SetTimer() in timer_handler. Note every SetTimer() is above a return;
void fake_timer_handler()
{
	//sweep LowPriQueue "HowManyTimes"
	if(elapsed-lastTimeBoost>=aHundredClockTicks){
		//accumulated how may times of calling inorderBoost
		HowManyTimes=(elapsed-lastTimeBoost)/aHundredClockTicks;//quotient . min:1
		out<<"...........Sweeping Low Priority queue.............\n";
		inorderBoost(LowPriTree,elapsed,HowManyTimes);
		out<<".................Sweeping Finished...................\n";
		lastTimeBoost+=aHundredClockTicks*HowManyTimes;
		
		#ifdef DEBUG
		temp++;
		//flush every 8 times we sweep
		if(temp%8==0){
			out.flush();
			cout<<"flushing...# of processes done: "<<finished.size()<<endl;
			
		}
		#endif
	}
	
  /*fill the current low, high band queue according to Arrival, which is the key of ArrTree */
  while (ArrTree!=NILPTR && elapsed>= (minimum(ArrTree)->Arrival)){
	    	  
		NODEPTR nextArrival=minimum(ArrTree);
			
		out<<"PID "<< nextArrival->Pid<<" arrives at "<< nextArrival->Arrival<<
		    " ms. Base Priority: "<< nextArrival->Pri <<", Burst: " << nextArrival->Burst <<"\n";
		
		
		//if is user process, connect it to LowPriTree by changing p and its neighbours' p, left, right
		if (nextArrival->Pri<50){
			rbdelete(&ArrTree,nextArrival);//nextArrival losts its p: Z->p=NILPTR
			rbinsertLowHigh(&LowPriTree,nextArrival);//nextArrival's p,left, and right changed.
			//the previous order can't change, otherwise in transplant, Z->p==NILPTR, so ArrTree become empty.
			LowPriTreeLength++;
		//if is high band process, connect it to highPriTree
		}else{
			rbdelete(&ArrTree,nextArrival);
			rbinsertLowHigh(&HighPriTree,nextArrival);
			HighPriTreeLength++;
		}

		ArrTreeLength--;
		if(ArrTreeLength==0) ArrTree=NILPTR;
		//last node in the ArrTree, make it NILPTR (nextArrival is already connected to High or Low queue)
		//if(maximum(ArrTree)==nextArrival) {
			//ArrTree=NILPTR;
			//ArrTreeLength=0;
			//cout<<ArrTreeLength<<"========="<<endl;//0
			//cout<<"LowPriTreeLength: "<<LowPriTreeLength<<", HighPriTreeLength:"<<HighPriTreeLength<<endl;
		//}
		 
		//not last node,do nothing
		//cout<<countNode(ArrTree)<<" in handler 3"<<endl;
  }
  
	 

  //time elapsed since the clock starts.  
  //cout<<elapsed<<" ms: "<<endl;
	
  /*run process with the highest DyPri (maxPri) at this clock tick. */ 
  NODEPTR maxPri=pickProcess();
  //there is nothing in both queues, jump to when next process arrives.
  if (maxPri==NILPTR) {
	  if (ArrTreeLength==0){return;}//if nothing is gonna arrive in the end, set no timer. main while loop exits.
	  //SetTimer(minimum(ArrTree)->Arrival-elapsed);
	  elapsed=minimum(ArrTree)->Arrival;
	  return;
  }
  //we have something in queues to run: 
  //if something was running and can't finish before the handler is called.
  if (Running!=NILPTR){
	  //if is not the same process as last one, output "... is kicked out..."
	  if(Running->LastTimeKickedOut!=maxPri->LastTimeKickedOut){
		
		out<<"(1) "<<elapsed<<" ms: \n"<<
		"(2) PID "<<Running->Pid<<"'s Quantum expires, remaining burst "<<Running->RemainBurst<<
		"\n(3) PID "<<maxPri->Pid<<" runs, remaining burst "<<maxPri->RemainBurst  <<"\n";
		
	  }
	  //else: if is the same process, output nothing.
  //if nothing is running right prior to this ms.
  }else{
	  out<<"PID "<<maxPri->Pid<<" runs at "<<elapsed<<"ms, remaining burst "<<maxPri->RemainBurst<<"\n";
  }
  //will not finish during this TIME_QUANTUM
  if(maxPri->RemainBurst>TIME_QUANTUM){

		elapsed+=TIME_QUANTUM;
		maxPri->LastTimeKickedOut=elapsed;
		//update RemainBurst
		maxPri->RemainBurst-=TIME_QUANTUM;
	  
		//decrease DyPri by the amount of clock ticks it spent in the CPU prior to the interrupt
		maxPri->DyPri-=TIME_QUANTUM;
		//process priority cannot go below its original base priority
		if(maxPri->DyPri<maxPri->Pri){
			maxPri->DyPri=maxPri->Pri;
		}
		//and is placed in its priority queue
		if (maxPri->Pri<50){
			rbFixNode(&LowPriTree,maxPri);
		}else{
			rbFixNode(&HighPriTree,maxPri);
		}
	  
		//=========save process pointer as "Running". only usage is to output kicked out PID
		Running=maxPri;
	  
		//set up timer. call the handler after TIME_QUANTUM ms.
		//SetTimer(TIME_QUANTUM);
		return;
  //will finish beforehand
  }else{
		maxPri->FinishTime=elapsed+maxPri->RemainBurst;
		out<<"PID "<<maxPri->Pid<<" finishes at "<<maxPri->FinishTime<<"ms"<<"\n";
		//update time to when next time the handler is called
		elapsed+=maxPri->RemainBurst;
		maxPri->RemainBurst=0;
		
		//detach the node for statistics. free after outputing statistics
		finished.push(maxPri);
		if((maxPri->Pri)<50){
			LowPriTreeLength--;
			rbdelete(&LowPriTree,maxPri);
		}else{
			HighPriTreeLength--;
			rbdelete(&HighPriTree,maxPri);
		}
		//=========update process pointer "Running"
		Running=NILPTR;
		//set up timer.
		//SetTimer(1);
		return;
  }

}


bool dupPids;

void releaseMM(){
	mm_release(&mm);
	if (dupPids){mm_release(&mm_int);}
}
 
int main()
{
	
	//NIL.left = NIL.right = NIL.p = NILPTR;
	//NIL.color = BLACK;

/*copying the whole file
ifstream  src("500processes", ios::binary);
ofstream  out("output.txt",   ios::binary);
dst << src.rdbuf();
*/	
	cout<<"Welcome to Hybrid Scheduler(RR+Priority).\n Output file is 'output.txt' \n Input the time quantum for RR in msec:"<<endl;
	cin>>TIME_QUANTUM;
	//cout<<TIME_QUANTUM<<endl;
	aHundredClockTicks=TIME_QUANTUM*100;//ms
	
	
	string ans;
	string str1 ("y");
	string str2 ("n");
	//question 0:--------------------------------------------------
	cout<<"the input file is? 1. 500processes 2. oneMillionProcesses 3. other"<<endl;
	int option;
	cin>>option;
	//not valid input
	while (option!=1 &&option!=2&&option!=3){
		cout<<"please enter 1, 2, or 3: "<<endl;
		cin>>option;
	}
	//infile--------------------------------------------------------
	string FileName="";
	ifstream infile; //+(string s);
	if (option==1){
		infile.open("500processes",ifstream::in);
	}else if(option==2){
		infile.open("oneMillionProcesses",ifstream::in);
	}else if (option==3){
		cout<<"specify the name of your input file(without quotation marks):"<<endl;
		cin>>FileName;
		//getline(cin, FileName);
		#ifdef DEBUG
		cout << "You entered: " << FileName << endl;
		#endif
		infile.open(FileName.c_str(),ifstream::in);
		//if (!infile.good()){}
		while (!infile){
			cout<<"input file not found or name contain invalid symbol, like '('. Specify the name again: "<<endl;
			cin>>FileName;
			infile.open(FileName.c_str(),ifstream::in);
        }
	}

	
	//question 1:--------------------------------------------------
	cout<<"Are there any duplicated PIDs? (y/n) If there are, we will keep track of Pids to sanitize the input, which costs memory. We will also maintain another red black tree for Pids to search and insert. "<<endl;
	cin>>ans;
	//cout<<ans<<endl;
	//scan in neither 'y' nor 'n'
	while (ans.compare(str1)!=0 && ans.compare(str2)!=0){
		cout<<"please enter 'y' or 'n': "<<endl;
		cin>>ans;
	}
	//scan in 'y' or 'n'
	if (ans.compare(str1) == 0){
		//is "y"
		dupPids=true;
	}else {
		//is "n"
		dupPids=false;
	}
	
	//question 2:---------------------------------------------------
	bool ignoreInvalidDataLine;
	cout<<"exit if the lines of the input file contains invalid symbols (e.g. not integer)? (y/n)"<<endl;
	cin>>ans;
	//scan in neither 'y' nor 'n'
	while (ans.compare(str1)!=0 && ans.compare(str2)!=0){
		cout<<"please enter 'y' or 'n': "<<endl;
		cin>>ans;
	}
	//scan in 'y' or 'n'
	if (ans.compare(str1) == 0){
		//is "y"
		ignoreInvalidDataLine=false;
	}else {
		//is "n"
		ignoreInvalidDataLine=true;
	}
		
	//question 3:---------------------------------------------------
	cout<<"Fast mode? (y/n) (fast mode doesn't use virtual timer at all. so much faster)"<<endl; 
	cin>>ans;
	
	//scan in neither 'y' nor 'n'
	while (ans.compare(str1)!=0 && ans.compare(str2)!=0){
		cout<<"please enter 'y' or 'n': "<<endl;
		cin>>ans;
	}
	//scan in 'y' or 'n'
	if (ans.compare(str1) == 0){
		//is "y"
		useTimer=false;
	}else {
		//is "n"
		useTimer=true;
	}



	//get total line #/node # to allocate memory----------------------------
	#ifdef DEBUG
	cout<<"Patient, counting input file lines."<<endl;
	#endif
	
	int nodeNo=0;
	char c;
    while (infile.get(c)){
		if (c == '\n') ++nodeNo;
	}
	#ifdef DEBUG
    cout<<"Number of lines/nodes to allocate: "<<nodeNo-1<<
    ". Doesn't sanitize the input file before allocate(this works well if the # of invalid lines is not much). However, if there are a lot of invalid lines and memory is not sufficient then add the sanitize functionality here."<<endl;
    #endif
   // cout<<sizeof(node)<<endl; 64
   // cout<<sizeof(node_int)<<endl;  40
	
	//go back to the beginning of the ifstream
    infile.clear();
	infile.seekg(0, ios::beg );
	
	
	//init mm:allocate nodeNo of node and node_int, respectively.-----------
	mm_init(&mm,nodeNo-1,sizeof(node));
	if (dupPids) {mm_init(&mm_int,nodeNo-1,sizeof(node_int));}
	
	#ifdef DEBUG
    cout<<"Init memory manager(allocating memory) succeeds."<<endl;
    #endif
	
	
	
	#ifdef DEBUG
	cout<<"Patient, processing every line..."<<endl;
	#endif
	
	int Pid, Burst, Arrival, Pri;
	string line;
	//# of lines in the input file(exclude the first line), # of invalid lines. 
	int fileLine=0, invalidLine=0;
	
	//start reading lines----------------------------------------------------
    if (!getline(infile,line)) {cout<<"Can't read the first line,exiting..."<<endl;releaseMM();exit(-1);}

    while (getline(infile,line))
    {
		fileLine++;
		istringstream iss(line);
		if (!(iss>>Pid>>Burst>>Arrival>>Pri)) { 
			invalidLine++;
			cout<<"can't read line "<< fileLine<<", format not right."<<endl; 
			if (!ignoreInvalidDataLine) {
				cout<<"choose to exit when encounter invalid line in input file. Exiting..."<<endl;	
				releaseMM();
				exit(-1);
			}else{
			//else ignoring this line
				#ifdef DEBUG
				cout<<"sanitized one line"<<endl;
				#endif
				continue;
			}
		}

		//sanitize and make arrival queue. i.e. sort + don't accept duplicated PID process
		
		//if there r dupPids, ignore the line of data with the same Pid which doesn't come first:
		if (dupPids && search(PIDs,Pid)!=NILPTR_int) {
				#ifdef DEBUG
				cout<<"sanitized one line"<<endl;
				#endif
				invalidLine++; 
				continue;
		}
		//if there isn't any dupPids:
		if (Burst>0&&Arrival>0&&Pid>0&&Pri<=99&&Pri>=0){
				if (dupPids){
					//allocate node_int to record existing PIDs
					NODEPTR_int Z;
					if (!(Z=(NODEPTR_int) mm_get(&mm_int))){
						perror("Failure to allocate memory:");
						cout<<"fails. exiting..."<<endl;
						releaseMM();
						exit(-1);				
					}
					Z->key = Pid;
					rbinsert_int(&PIDs,Z);//ith node in memory manager.
				}
				
				//allocate node to add to ArrTree
				NODEPTR T;
				if (!(T=(NODEPTR)mm_get(&mm))){
					perror("Failure to allocate memory:");
					cout<<"fails. exiting..."<<endl;
					releaseMM();
					exit(-1);				
				}
				T->Arrival = Arrival; 
				T->DyPri=T->Pri=Pri;//z=Pri
				T->Pid =Pid;
				T->RemainBurst=T->Burst=Burst;
				T->LastTimeKickedOut=-1;//haven't seen the CPU yet
				rbinsertArrTree(&ArrTree, T);
		}else{
			#ifdef DEBUG
			cout<<"sanitized one line"<<endl;
			#endif
			invalidLine++;
			continue;
		}
    }
    
    infile.close();
    
    #ifdef DEBUG
	cout<<"Input file contains "<<fileLine<<" lines (excluding the first line), "
	<<invalidLine<<" lines of which are invalid.\n"<<"Done sorting Arrival queue.\n";
	#endif
	
	int ArrTreeLength_copy=ArrTreeLength=fileLine-invalidLine;//498
	if (ArrTreeLength==0) {cout<<"Exit. no valid lines exist."<<endl; exit(-1);}
    
    //deallocate red black tree.
    if (dupPids){
		mm_release(&mm_int);
		PIDs=NILPTR_int;
	}
    //in order walk: arrival queue
    //inorder(ArrTree);
    
    
	//cout<<countNode(ArrTree)<<"\n===============================================\n"<<endl;
	
	
	

	//====================output with file==============================
	
	out.open("output.txt");
	if (out.is_open()){
		#ifdef DEBUG
		 cout<<"Opened output.txt"<<endl;
		 #endif
		//out<< "Writing this to a file.\n";
		//out.flush();
	}else{
		cout<<"Unable to open output.txt, exiting...\n"; exit(-1);
	}
	//get stuck at "endl":
	//out<<"qq"<<endl;
	cout<<"Start scheduling...Flushing info to 'output.txt'..."<<endl;
	//set current time to the first process arrival time.
	elapsed=minimum(ArrTree)->Arrival;
	
 if (useTimer){
    struct sigaction sa; 
  
	//==============================================================
	/* Install timer_handler as the signal handler for SIGVTALRM.  */ 
	memset (&sa, 0, sizeof (sa)); 
	sa.sa_handler = &timer_handler; 
	sigaction (SIGVTALRM, &sa, NULL); 
  
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 0;//microseconds
	timer.it_value.tv_sec = 0; 
	timer.it_value.tv_usec=1;

 
	//===========================================================
	//memset (&sa2, 0, sizeof (sa2)); 
	//sa2.sa_handler = &timer_handler2; 
	//sigaction (SIGVTALRM, &sa2, NULL); 
	
	//timer2.it_interval.tv_sec = 0;
	//timer2.it_interval.tv_usec = TIME_QUANTUM*100*1000;//microseconds
	//timer2.it_value.tv_sec = 0; 
	//timer2.it_value.tv_usec=TIME_QUANTUM*100*1000;//one hundred clock ticks(ms)*1000=us
	////==========================================================
	////call the handler after "elapsed" ms.
}
	struct timeval time_s, time_e;

  /* start another timer */
  gettimeofday (&time_s, NULL);


	if(useTimer) {
		SetTimer(elapsed); 
		while (ArrTreeLength!=0||LowPriTreeLength!=0||HighPriTreeLength!=0){ }
   }else{
		while (ArrTreeLength!=0||LowPriTreeLength!=0||HighPriTreeLength!=0){
			fake_timer_handler();
		}
   }
	
	////call the timer_hander2 every 100 clock ticks
	//setitimer(ITIMER_VIRTUAL, &timer2,NULL); /* Start a virtual timer.*/
	//====================================================================
  

  //get here if every tree is empty. exit.
    cout<<"................Done scheduling.................\n # of processess scheduled: "<<finished.size()<<endl;
     gettimeofday(&time_e, NULL);
     cout<<"Time taken = "<<comp_time(time_s, time_e) / 1000000.0 <<" sec\n"; 
  
   //statistics:
   //free
   NODEPTR Z;
   int turnaround, waiting;
   long long turnaround_ALL=0,waiting_ALL=0;//ms
   cout<<"..............Outputing statistics............"<<endl;
   out<<"================Statistics===========================\n";
   out<<"Pid          turnaround time(msec)          waiting time(msec) \n";

   while (!finished.empty()){
	    Z=finished.front();
	    finished.pop();
	    turnaround=Z->FinishTime-Z->Arrival;
	    waiting=turnaround-Z->Burst;
	    //out<<"Pid "<<Z->Pid<<" turnaround time: "<<turnaround<<", waiting time: "<<waiting<<"\n";
	    out<<Z->Pid<<"           		"<<turnaround<<"    		     "<<waiting; // for testing: " Arrival: "<<Z->Arrival<<" FinishTime"<<Z->FinishTime ;
	    turnaround_ALL+=turnaround;
	    out<<"   ,turnaround_ALL: "<<turnaround_ALL<<"\n";
	    waiting_ALL+=waiting;
	}
	mm_release(&mm);
	
	cout<<"Succeed. Finished outputing statistics. \n"<<
		"Conclusion: Average turnaround time: "<<turnaround_ALL/ArrTreeLength_copy<<
		" msec,\n average waiting time: "<<waiting_ALL/ArrTreeLength_copy<<
		" msec, \n number of processes scheduled: "<<ArrTreeLength_copy<<"\n All succeeds. Exiting... "<<endl;
	out<<"......................................................\n";
	out<<"Average turnaround time: "<<turnaround_ALL/ArrTreeLength_copy<<
		" msec, \n average waiting time: "<<waiting_ALL/ArrTreeLength_copy<<
		" msec, \n number of processes scheduled: "<<ArrTreeLength_copy<<"\n";
    out.close();
    return EXIT_SUCCESS;
}


