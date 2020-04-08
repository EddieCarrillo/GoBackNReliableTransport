#include <stdio.h>

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose
   This code should be used for PA2, unidirectional or bidirectional
   data transfer protocols (from A to B. Bidirectional transfer of data
   is for extra credit and is not required).  Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

#define BIDIRECTIONAL 0    /* change to 1 if you're doing extra credit */
                           /* and write a routine called B_output */
#define SENDER 0
#define RECEIVER 1
#define MSG_LEN 20 //Symbolic constant for max size of message.
#define TIMER_LEN 20.0f //Being very conservative
#define SND_BUF_LEN 50

/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
struct msg {
  char data[20];
  };

/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
struct pkt {
   int seqnum;
   int acknum;
   int checksum;
   char payload[20];
    };

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/


/*Sender states*/
int base = 1;
int nextSeqNum = 1;
struct pkt sndBuf[SND_BUF_LEN];

/*Receiver states*/
int expectedSeqNumber = 1;
struct pkt lastAckedPacket;
/* Transport layer sender recieived data from upper layer to send to network layer */
void A_output(struct msg message)
{
   //Refuse Data
    if (waitingForAck()){
        printf("The sender is currently busy waiting for a packet, dropping the data from the application layer.");
        return;
    }
    printf("Creating packet and sending it to the network layer");
    //WaitingForAck == 0 Not waiting for an ACK, thus go ahead and process + send the data.
    createAndSendSenderPacket(message);
    printf("Created packet and sent it to the network layer.");
    //Added packet to sndbuf window
    //Start the timer 
    printf("Starting the packet loss timer.\n");
    if (base == nextSeqNum) // This means we have a new window.
        starttimer(SENDER, TIMER_LEN);
    ++nextSeqNum; //The sequence of the packet we would send next if there is enough room in the window.
}

void B_output(struct msg message)  /* need be completed only for extra credit */
{

}

/* transport layer send, receives data from the network layer. */
void A_input(struct pkt packet)
{
   //Corupt ACK, wrong ACK, is a NACK (use negative values for NACK)
  if (isCorrupt(SENDER, packet)){
      printf("The data was corrupted do nothing\n");
      return;
  }
  //At this point we have that we have received an ACK so...
  printf("Received an good ACK from the reciever, can now get new data to send\n");
  //Update the base
  int shiftLift = packet.acknum - base + 1; //Cumulative Acks.
  shiftLeftByN(sndBuf, SND_BUF_LEN, shiftLift); //make acknum + 1 the new base
  base = packet.acknum + 1;
  if (base == nextSeqNum) //We are no longer waiting for any ACKs
      stoptimer(SENDER); 
  else 
      starttimer(SENDER);
     
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
   printf("TIMER!!! \n");
   //At this point we should resend the data and reset the timer.
   printf("Resending packets.\n");
   int i;
   for (i = 0; i < SND_BUF_LEN; i++){
      tolayer3(SENDER,sndBuf[i]);
   }
   printf("Resent packets from sndbuf.\n");
   //Reset the timer.
   printf("Restarting the timer\n");
   starttimer(SENDER, TIMER_LEN);
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
}

struct pkt createReceiverPacket(int acknum){
   struct pkt sndPkt;
   sndPkt.acknum = acknum;
   sndPkt.checksum = computeChecksum(RECEIVER, sndPkt);
   return sndPkt;
}

void createReceiverPacketAndSend(int acknum){
    tolayer3(RECEIVER, createReceiverPacket(acknum));
}

/* called when B's timer goes off */
void B_timerinterrupt()
{
}
/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */

void B_init()
{
   lastAckedPacket.seqnum = -1; //Initially there is not packet to resent (EDGE CASE)
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* Transport layer receiver gets data from network layer.*/
void B_input(struct pkt packet)
{
   if (isCorrupt(RECEIVER, packet)){
      printf("Received a corrupt packet on the receiver side.\n");
      if (lastAckedPacket.seqnum > 0){
         tolayer3(RECEIVER, lastAckedPacket);
      }
      return;
   }
   if (packet.seqnum != expectedSeqNumber){
      printf("Received out of order packet.\n");
      if (lastAckedPacket.seqnum > 0){
         tolayer3(RECEIVER, lastAckedPacket);
      }
      return;
   }

   //Not corrupt and in order.
   //Extract data
   struct msg message;
   int i;
   for (i = 0; i < MSG_LEN; i++)
       (message.data)[i] = (packet.payload)[i];
       //Send it to higher layer
   tolayer5(RECEIVER, message);
   printf("Extracted data and sent it up.\n");
   //Send an ACK
   lastAckedPacket = createReceiverPacket(expectedSeqNumber);
   tolayer5(RECEIVER, lastAckedPacket);
   printf("Created ACK and sent it to the sender.\n");
   //Update the expected seq number
   expectedSeqNumber++; 
}


struct pkt createSenderPacket(struct msg message){
   struct pkt sndpkt;
    //Set the sequence number
   sndpkt.seqnum = nextSeqNum;
    //Copy the message data into the payload field for the packet
   int i;
   for (i = 0; i < MSG_LEN && message.data[i] != '\0'; i++)
      (sndpkt.payload)[i] = message.data[i];
    //Compute the checksum
   sndpkt.checksum = computeChecksum(SENDER, sndpkt);
   return sndpkt;
}

void createAndSendSenderPacket(struct msg message){
    printf("Created the packet.\n");
    //Here the seqnum can go up to 2**31 - 1
    sndBuf[nextSeqNum-base] = createSenderPacket(message);
    //Packet is created, now we can send it into the network layer.
    printf("Sending the packet\n");
    tolayer3(SENDER, sndBuf[nextSeqNum - base]);
}

int computeChecksum(int packetType, struct pkt packet)
{
    int sum = 0;
    int i;
    if (packetType == SENDER){
      for (i = 0; i < MSG_LEN && packet.payload[i] != '\0'; i++)
	        sum += packet.payload[i];
       sum += packet.seqnum;
    }
    if (packetType == RECEIVER)
        sum += packet.acknum;
    return ~sum;
}

int waitingForAck(){
   return nextSeqNum >= base + SND_BUF_LEN;
}

/*Converts 32 bit integer into C str*/
char* toBinary (int x)
{
    char* msg = (char*) malloc(33* sizeof(char));
    msg[32] = '\0';
    int i;
    for (i = 0; i < 32; i++){
        msg[31-i] = '0' + ((x >> i) & 1);
    }
     return msg;
}

int isCorrupt(int packetType, struct pkt packet)
{
    int expectedChecksum = packet.checksum;
    int calculatedChecksum = 0;
    int msgLen = 20; 
    int i;
    //The receiver is expecting a payload and seq_num
    if (packetType == RECEIVER){
       for (i = 0; i < msgLen; i++)
        calculatedChecksum += (packet.payload)[i];
        calculatedChecksum += packet.seqnum;
    }
    //The sender side is just expecting the ACK
    if (packetType == SENDER){
       calculatedChecksum += packet.acknum;
    }
        
    char* calculated = toBinary(calculatedChecksum);
    char* expected = toBinary(expectedChecksum);
    char* sum = toBinary(calculatedChecksum + expectedChecksum);

    printf("calculated sum: %s\n", calculated);
    printf("packet checksum: %s\n", expected);
    printf("sum: %s\n", sum);

    free(calculated);
    free(expected);
    free(sum);

    return ((expectedChecksum + calculatedChecksum) != (0xFFFFFFFF));
}

/*This should be replaced with some kind of ciruclar buffer.*/
void shiftLeftByN(int arr [] , int arrlen, int n){
    int i;
    for (i = n; i < arrlen; i++){
        arr[i-n] = arr[i]; //Shift left by n.
    }
}


/*****************************************************************
***************** NETWORK EMULATION CODE STARTS BELOW ***********
The code below emulates the layer 3 and below network environment:
  - emulates the tranmission and delivery (possibly with bit-level corruption
    and packet loss) of packets across the layer 3/4 interface
  - handles the starting/stopping of a timer, and generates timer
    interrupts (resulting in calling students timer handler).
  - generates message to be sent (passed from later 5 to 4)
THERE IS NOT REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOLD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should have
to, and you defeinitely should not have to modify
******************************************************************/

struct event {
   float evtime;           /* event time */
   int evtype;             /* event type code */
   int eventity;           /* entity where event occurs */
   struct pkt *pktptr;     /* ptr to packet (if any) assoc w/ this event */
   struct event *prev;
   struct event *next;
 };
struct event *evlist = NULL;   /* the event list */

/* possible events: */
#define  TIMER_INTERRUPT 0  
#define  FROM_LAYER5     1
#define  FROM_LAYER3     2

#define  OFF             0
#define  ON              1
#define   A    0
#define   B    1



int TRACE = 1;             /* for my debugging */
int nsim = 0;              /* number of messages from 5 to 4 so far */ 
int nsimmax = 0;           /* number of msgs to generate, then stop */
float time = 0.000;
float lossprob;            /* probability that a packet is dropped  */
float corruptprob;         /* probability that one bit is packet is flipped */
float lambda;              /* arrival rate of messages from layer 5 */   
int   ntolayer3;           /* number sent into layer 3 */
int   nlost;               /* number lost in media */
int ncorrupt;              /* number corrupted by media*/

main()
{
   struct event *eventptr;
   struct msg  msg2give;
   struct pkt  pkt2give;
   
   int i,j;
   char c; 
  
   init();
   A_init();
   B_init();
   
   while (1) {
        eventptr = evlist;            /* get next event to simulate */
        if (eventptr==NULL)
           goto terminate;
        evlist = evlist->next;        /* remove this event from event list */
        if (evlist!=NULL)
           evlist->prev=NULL;
        if (TRACE>=2) {
           printf("\nEVENT time: %f,",eventptr->evtime);
           printf("  type: %d",eventptr->evtype);
           if (eventptr->evtype==0)
	       printf(", timerinterrupt  ");
             else if (eventptr->evtype==1)
               printf(", fromlayer5 ");
             else
	     printf(", fromlayer3 ");
           printf(" entity: %d\n",eventptr->eventity);
           }
        time = eventptr->evtime;        /* update time to next event time */
        if (nsim==nsimmax)
	  break;                        /* all done with simulation */
        if (eventptr->evtype == FROM_LAYER5 ) {
            generate_next_arrival();   /* set up future arrival */
            /* fill in msg to give with string of same letter */    
            j = nsim % 26; 
            for (i=0; i<20; i++)  
               msg2give.data[i] = 97 + j;
            if (TRACE>2) {
               printf("          MAINLOOP: data given to student: ");
                 for (i=0; i<20; i++) 
                  printf("%c", msg2give.data[i]);
               printf("\n");
	     }
            nsim++;
            if (eventptr->eventity == A) 
               A_output(msg2give);  
             else
               B_output(msg2give);  
            }
          else if (eventptr->evtype ==  FROM_LAYER3) {
            pkt2give.seqnum = eventptr->pktptr->seqnum;
            pkt2give.acknum = eventptr->pktptr->acknum;
            pkt2give.checksum = eventptr->pktptr->checksum;
            for (i=0; i<20; i++)  
                pkt2give.payload[i] = eventptr->pktptr->payload[i];
	    if (eventptr->eventity ==A)      /* deliver packet by calling */
   	       A_input(pkt2give);            /* appropriate entity */
            else
   	       B_input(pkt2give);
	    free(eventptr->pktptr);          /* free the memory for packet */
            }
          else if (eventptr->evtype ==  TIMER_INTERRUPT) {
            if (eventptr->eventity == A) 
	       A_timerinterrupt();
             else
	       B_timerinterrupt();
             }
          else  {
	     printf("INTERNAL PANIC: unknown event type \n");
             }
        free(eventptr);
        }

terminate:
   printf(" Simulator terminated at time %f\n after sending %d msgs from layer5\n",time,nsim);
}



init()                         /* initialize the simulator */
{
  int i;
  float sum, avg;
  float jimsrand();
  
  
   printf("-----  Stop and Wait Network Simulator Version 1.1 -------- \n\n");
   printf("Enter the number of messages to simulate: ");
   scanf("%d",&nsimmax);
   printf("Enter  packet loss probability [enter 0.0 for no loss]:");
   scanf("%f",&lossprob);
   printf("Enter packet corruption probability [0.0 for no corruption]:");
   scanf("%f",&corruptprob);
   printf("Enter average time between messages from sender's layer5 [ > 0.0]:");
   scanf("%f",&lambda);
   printf("Enter TRACE:");
   scanf("%d",&TRACE);

   srand(9999);              /* init random number generator */
   sum = 0.0;                /* test random number generator for students */
   for (i=0; i<1000; i++)
      sum=sum+jimsrand();    /* jimsrand() should be uniform in [0,1] */
   avg = sum/1000.0;
   if (avg < 0.25 || avg > 0.75) {
    printf("It is likely that random number generation on your machine\n" ); 
    printf("is different from what this emulator expects.  Please take\n");
    printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
    exit(0);
    }

   ntolayer3 = 0;
   nlost = 0;
   ncorrupt = 0;

   time=0.0;                    /* initialize time to 0.0 */
   generate_next_arrival();     /* initialize event list */
}

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand() 
{
  double mmm = 2147483647;   /* largest int  - MACHINE DEPENDENT!!!!!!!!   */
  float x;                   /* individual students may need to change mmm */ 
  x = rand()/mmm;            /* x should be uniform in [0,1] */
  return(x);
}  

/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/
 
generate_next_arrival()
{
   double x,log(),ceil();
   struct event *evptr;
    char *malloc();
   float ttime;
   int tempint;

   if (TRACE>2)
       printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");
 
   x = lambda*jimsrand()*2;  /* x is uniform on [0,2*lambda] */
                             /* having mean of lambda        */
   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtime =  time + x;
   evptr->evtype =  FROM_LAYER5;
   if (BIDIRECTIONAL && (jimsrand()>0.5) )
      evptr->eventity = B;
    else
      evptr->eventity = A;
   insertevent(evptr);
} 


insertevent(p)
   struct event *p;
{
   struct event *q,*qold;

   if (TRACE>2) {
      printf("            INSERTEVENT: time is %lf\n",time);
      printf("            INSERTEVENT: future time will be %lf\n",p->evtime); 
      }
   q = evlist;     /* q points to header of list in which p struct inserted */
   if (q==NULL) {   /* list is empty */
        evlist=p;
        p->next=NULL;
        p->prev=NULL;
        }
     else {
        for (qold = q; q !=NULL && p->evtime > q->evtime; q=q->next)
              qold=q; 
        if (q==NULL) {   /* end of list */
             qold->next = p;
             p->prev = qold;
             p->next = NULL;
             }
           else if (q==evlist) { /* front of list */
             p->next=evlist;
             p->prev=NULL;
             p->next->prev=p;
             evlist = p;
             }
           else {     /* middle of list */
             p->next=q;
             p->prev=q->prev;
             q->prev->next=p;
             q->prev=p;
             }
         }
}

printevlist()
{
  struct event *q;
  int i;
  printf("--------------\nEvent List Follows:\n");
  for(q = evlist; q!=NULL; q=q->next) {
    printf("Event time: %f, type: %d entity: %d\n",q->evtime,q->evtype,q->eventity);
    }
  printf("--------------\n");
}



/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
stoptimer(AorB)
int AorB;  /* A or B is trying to stop timer */
{
 struct event *q,*qold;

 if (TRACE>2)
    printf("          STOP TIMER: stopping timer at %f\n",time);
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
 for (q=evlist; q!=NULL ; q = q->next) 
    if ( (q->evtype==TIMER_INTERRUPT  && q->eventity==AorB) ) { 
       /* remove this event */
       if (q->next==NULL && q->prev==NULL)
             evlist=NULL;         /* remove first and only event on list */
          else if (q->next==NULL) /* end of list - there is one in front */
             q->prev->next = NULL;
          else if (q==evlist) { /* front of list - there must be event after */
             q->next->prev=NULL;
             evlist = q->next;
             }
           else {     /* middle of list */
             q->next->prev = q->prev;
             q->prev->next =  q->next;
             }
       free(q);
       return;
     }
  printf("Warning: unable to cancel your timer. It wasn't running.\n");
}


starttimer(AorB,increment)
int AorB;  /* A or B is trying to stop timer */
float increment;
{

 struct event *q;
 struct event *evptr;
 char *malloc();

 if (TRACE>2)
    printf("          START TIMER: starting timer at %f\n",time);
 /* be nice: check to see if timer is already started, if so, then  warn */
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
   for (q=evlist; q!=NULL ; q = q->next)  
    if ( (q->evtype==TIMER_INTERRUPT  && q->eventity==AorB) ) { 
      printf("Warning: attempt to start a timer that is already started\n");
      return;
      }
 
/* create future event for when timer goes off */
   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtime =  time + increment;
   evptr->evtype =  TIMER_INTERRUPT;
   evptr->eventity = AorB;
   insertevent(evptr);
} 


/************************** TOLAYER3 ***************/
tolayer3(AorB,packet)
int AorB;  /* A or B is trying to stop timer */
struct pkt packet;
{
 struct pkt *mypktptr;
 struct event *evptr,*q;
 char *malloc();
 float lastime, x, jimsrand();
 int i;


 ntolayer3++;

 /* simulate losses: */
 if (jimsrand() < lossprob)  {
      nlost++;
      if (TRACE>0)    
	printf("          TOLAYER3: packet being lost\n");
      return;
    }  

/* make a copy of the packet student just gave me since he/she may decide */
/* to do something with the packet after we return back to him/her */ 
 mypktptr = (struct pkt *)malloc(sizeof(struct pkt));
 mypktptr->seqnum = packet.seqnum;
 mypktptr->acknum = packet.acknum;
 mypktptr->checksum = packet.checksum;
 for (i=0; i<20; i++)
    mypktptr->payload[i] = packet.payload[i];
 if (TRACE>2)  {
   printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
	  mypktptr->acknum,  mypktptr->checksum);
    for (i=0; i<20; i++)
        printf("%c",mypktptr->payload[i]);
    printf("\n");
   }

/* create future event for arrival of packet at the other side */
  evptr = (struct event *)malloc(sizeof(struct event));
  evptr->evtype =  FROM_LAYER3;   /* packet will pop out from layer3 */
  evptr->eventity = (AorB+1) % 2; /* event occurs at other entity */
  evptr->pktptr = mypktptr;       /* save ptr to my copy of packet */
/* finally, compute the arrival time of packet at the other end.
   medium can not reorder, so make sure packet arrives between 1 and 10
   time units after the latest arrival time of packets
   currently in the medium on their way to the destination */
 lastime = time;
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
 for (q=evlist; q!=NULL ; q = q->next) 
    if ( (q->evtype==FROM_LAYER3  && q->eventity==evptr->eventity) ) 
      lastime = q->evtime;
 evptr->evtime =  lastime + 1 + 9*jimsrand();
 


 /* simulate corruption: */
 if (jimsrand() < corruptprob)  {
    ncorrupt++;
    if ( (x = jimsrand()) < .75)
       mypktptr->payload[0]='Z';   /* corrupt payload */
      else if (x < .875)
       mypktptr->seqnum = 999999;
      else
       mypktptr->acknum = 999999;
    if (TRACE>0)    
	printf("          TOLAYER3: packet being corrupted\n");
    }  

  if (TRACE>2)  
     printf("          TOLAYER3: scheduling arrival on other side\n");
  insertevent(evptr);
} 

tolayer5(AorB,datasent)
  int AorB;
  char datasent[20];
{
  int i;  
  if (TRACE>2) {
     printf("          TOLAYER5: data received: ");
     for (i=0; i<20; i++)  
        printf("%c",datasent[i]);
     printf("\n");
   }
  
}