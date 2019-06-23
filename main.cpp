#include "mbed.h"
#include "rtos.h"
#include "stdlib.h"
#include "time.h"

#define THREAD_NUM 5
#define CRASHED 1000


/* Mail */
typedef struct {
  int  voto; 
} mail_t;
 
Serial pc(USBTX,USBRX,9600);
Mail<mail_t, THREAD_NUM-1> mail_box[THREAD_NUM];

void populate_ping_array(int,int,int[],int[]);
void calculate_min_ping(int[],int,int,int[]);
void send(int[],int,int[]);
void receive(int,int[],int,int[]);
int calculate_winner(int,int[],int,int[]);

void activities(int *myid) {
    
    pc.printf("Sono il thread %d\n\r",*myid);
    
    int i;
    
    int others[THREAD_NUM]; //vettore dove so l'id di tutti i thread compreso il mio! 
    for(i = 0; i < THREAD_NUM; i++){
        if(i == *myid)
            others[i] = -1;    //al mio posto metto -1 così so che gli altri sono caratterizzati da id >= 0
        else
            others[i] = i;
    }
    
    int winner;
    
    int my_votes[THREAD_NUM]; // vettore dei voti
    for(int i = 0; i < THREAD_NUM; i++){
        my_votes[i] = THREAD_NUM;    //Tutti le locazioni sono inizializzate al numero non significativo 5-5-5-5-5
    }
    
      
    int actives = THREAD_NUM-1; //numero di nodi attivi e non crashati
    
    int my_ping_times[THREAD_NUM-1] = {4,3,999,5};
    
    int results[actives+1];
     
    
    /*for(int i = 0; i < THREAD_NUM-1; i++){
        my_ping_times[i] = 0;    
    }*/
    
    populate_ping_array(*myid,actives,my_votes,my_ping_times);
    
    if(*myid != 3){
     
        calculate_min_ping(my_votes,*myid,actives,my_ping_times);
    
        send(others,*myid,my_votes);    
    
    //Thread::wait(3000);   
             
        receive(*myid,my_votes,actives,results);
    
        winner = calculate_winner(*myid,my_votes,actives,results); 
    
      
        pc.printf("Thread %d riconosce come vincente il thread %d\n\r",*myid,winner);  
    
    }      

}

 
int main (void) {
    
    Thread thread[THREAD_NUM];
    int i; 
    
    pc.printf("Creazione pool di thread\n\r"); 
            
    for(i = 0; i < THREAD_NUM; i++){
       thread[i].start(callback(activities,&i));
       Thread::wait(500);
     }      
     
         
    for(i = 0; i < THREAD_NUM; i++)
       thread[i].join();         
        
    pc.printf("Bye bye\n\r");
    
    return 0;
    
}

void populate_ping_array(int myid,int actives,int my_votes[],int my_ping_times[]){
    
    srand(time(NULL));    
      
    int i;
    if(myid == 3){
        for(i = 0; i < THREAD_NUM-1; i++)
            my_ping_times[i] = 999;    
    }
    
    if(myid == 4){
         my_ping_times[0] = 4; 
         my_ping_times[1] = 3; 
         my_ping_times[2] = 5; 
         my_ping_times[3] = 999;      
    }
    //simula il ping!
    for(i = 0; i < THREAD_NUM-1; i++){
        
        //my_ping_times[i] = 995+rand()%4;
        /*Scrive nel vettore dei miei voti qual è il nodo di cui non è riuscito a calcolare il ping e setta la sua locazione a -1*/
        if(my_ping_times[i] >= 999){
        
            if(i >= myid)
                my_votes[i+1] = CRASHED; //metto un valore alto per far capire che il nodo è crashato
            else
                my_votes[i] = CRASHED;
                        
            actives--; //diminuisco il conteggio dei nodi attivi
                    
        }
    }
    
    pc.printf("Il thread %d riconosce %d thread attivi\n\r",myid,actives);
    
    //for(i = 0; i < THREAD_NUM-1; i++)
    //    pc.printf("my_ping_times[%d] = %d\n\r",i,my_ping_times[i]); 
    
    
}

void calculate_min_ping(int my_votes[],int myid,int actives,int my_ping_times[]){
       
    int i;        
    //cerca il minimo fra i ping ed il nodo che l'ha prodotto
    int min = my_ping_times[0];
    int min_index = 0;
    for(i = 1; i < THREAD_NUM-1; i++){
        if(my_ping_times[i] < min){
            min = my_ping_times[i];
            min_index = i;
        }              
    }
        
    //inserisce il nodo con il ping minimo fra la lista dei votati nel suo indice.
    if(min_index >= myid){
        //votes[min_index+1] = min_index;
        my_votes[myid] = min_index+1;
        //pc.printf("Thread %d dice che il ping minimo e' del thread %d\n\r",myid,min_index+1);
    }
    else{
        //votes[min_index] = min_index;
        my_votes[myid] = min_index;
        //pc.printf("Thread %d dice che il ping minimo e' del thread %d\n\r",myid,min_index);
    }      

    
}

void send(int others[], int myid, int my_votes[]){
    
    int i;
                
    for(i = 0; i < THREAD_NUM; i++){
        if(others[i] == -1)
            continue;
        else if(my_votes[i] == CRASHED)
            continue;
        else{            
                mail_t *mail = mail_box[others[i]].alloc();
                               
                mail->voto = my_votes[myid]; //il thread mette il proprio voto nella mail e la spedisce a tutti i nodi !crash
                      
                mail_box[others[i]].put(mail);
                                              
                //pc.printf("Il thread %d ha appena inviato una mail al thread %d \n\r",myid,others[i]);            
                 
        }   
        //Thread::wait(200);  
    }  
   
}

void receive(int myid,int my_votes[],int actives,int results[]){
    
    int i = 1;     
    results[0] = my_votes[myid];
    int counter = 0;    
 
    for(int j = 0; j < actives-1; j++){        
        osEvent evt = mail_box[myid].get();
        mail_t *mail = (mail_t*)evt.value.p;
        if(evt.status == osEventMail){
            pc.printf("Il thread %d ha ricevuto una mail con contenuto %d\n\r",myid,mail->voto);
            results[i] = mail->voto;
            i++;                                                   
            mail_box[myid].free(mail);
            counter++;
        }
        //if(counter == actives)
           // break;
                              
    }
    
    for(i = 0; i < actives; i++)
        pc.printf("Thread %d results[%d] = %d\n\r",myid,i,results[i]);
    
}


int calculate_winner(int myid,int my_votes[],int actives,int results[]){
          
    int max_count = 0;
    int tmp_winners[actives+1];
    int winner = -1;
    int i,j;      
    
    for (i = 0; i < actives+1; i++){
        int count=1;
        for (j = i + 1; j < actives+1; j++){
            if (results[i] == results[j]){
                count++;
                //freq[j] = 0;
            }
        }
        if(count>max_count){
            max_count = count;          
            tmp_winners[i] = results[i];
        }                  
    }
    
    winner = tmp_winners[0];
   
    for (i = 1; i < actives+1; i++)
    {
        if (tmp_winners[i] < winner)
        {
           winner = tmp_winners[i];
           
        }
    }    
                
    return winner;
    
}
