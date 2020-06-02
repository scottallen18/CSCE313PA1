/*
    Scott Allen
    Department of Computer Science & Engineering
    Texas A&M University
    Date  : 5/29/20
 */
#include "common.h"
#include "FIFOreqchannel.h"
#include <getopt.h>
#include <sys/time.h>
#include <iostream>
#include <fstream>
#include <sys/wait.h>
using namespace std;


int main(int argc, char *argv[]){
    /*
    RUN THE SERVER AS A CHILD PROCESS (15 PTS)
    Run the server process as a child of the client process using 
    fork and exec() such that you don need two terminals. 1 for 
    the client and another for the server. The outcome is that you 
    open a single terminal run the server then connects to it. Also, 
    to make sure that the the server does not keep running after the 
    client dies, sent a special QUIT_MSG to the server and call wait() 
    funciton to wait for its finish.
    */
   pid_t pid = fork();
   int status;
   if(pid == 0){
        cout << "Starting client as the parent process pid = "<< pid << endl;          
        int option;
        //variables to tell us how to handle the arugments
        // these will be modified if the getopt function is triggered
        bool pdef = 0;
        bool tdef = 0;
        bool edef = 0;
        bool fdef = 0;
        bool cdef = 0;
        //default variables
        int patient_num = 1;
        double ecg_time = 0.0000;
        int ecg = 1;
        string filename = "1.csv";
        while(( option = getopt(argc, argv, "p:t:e:f:c")) != -1){
            switch (option){
                case 'p':
                    //patient defined in the argument
                    pdef = 1;
                    patient_num = atoi(optarg);
                    break;
                case 't':
                    //time defined in the argument
                    tdef = 1;
                    ecg_time = atof(optarg);
                    break;
                case 'e':
                    // ECG defined in the argument
                    edef = 1;
                    ecg = atoi(optarg);
                    break;                                        
                case 'f':
                    //file defined in the argument
                    fdef = 1;
                    filename = optarg;
                    break;
                case 'c':
                    //new channel requested in the argument
                    cdef = 1;
                    break;
            }
        }
        
        //GETTING DATA POINT (15 PTS)
        if(pdef || tdef || edef){
            FIFORequestChannel chan ("control", FIFORequestChannel::CLIENT_SIDE);
                cout << "Input Recieved"<< endl;
                cout << "Patient Number : " << patient_num;
                cout <<" Time : "<< ecg_time;
                cout <<" ECG : " << ecg<<endl;
                
                datamsg *data = new datamsg( patient_num, ecg_time, ecg);
                chan.cwrite(data, sizeof(datamsg));
                double recived_val = 0;
                chan.cread( &recived_val, sizeof(double));
                cout << recived_val << endl;
            /*
            if(patient_num == 0){
                TESTING PURPOSE 
                Request at least 1000 data points for a person (both ecg1 and ecg2)
                collect the responces, and put them in a file called x1.csv. 
                Compare the file against correspond data pionts in the original file
                and demonstrate that they match. ALso measure the time for collecing 
                data points using gettingofday funciton, which has a microsecond granulity
                
                
                //ofstream myfile;
                //myfile.open("x1.csv");
                int datapoints_taken = 0;
                for(int X = 10; X <= 60; X +=5){
                    struct timeval start, end;
                    gettimeofday( &start, NULL);
                    for(double time = 0.000; time < X; time += .004){
                        datapoints_taken ++;
                            //Grab the data points from the server
                            datamsg *ecg1 = new datamsg( 1, time, 1);
                            chan.cwrite(ecg1, sizeof(datamsg));
                            double data1 = 0;
                            chan.cread( &data1, sizeof(double));
                            
                            datamsg *ecg2 = new datamsg( 1, time, 2);
                            chan.cwrite(ecg2, sizeof(datamsg));
                            double data2 = 0;
                            chan.cread(&data2, sizeof(double));

                            //Write to the file
                            //myfile << time <<","<<data1<<","<<data2<<endl;                        
                    }
                    //myfile.close();
                    gettimeofday( &end, NULL);
                    double time_taken = (end.tv_sec - start.tv_sec);
                    cout << "Time_taken to get "<< datapoints_taken<< " points : " << time_taken << endl;
                }
            }
            */
           
            // closing the channel  (5 PTS)  
            MESSAGE_TYPE m = QUIT_MSG;
            chan.cwrite (&m, sizeof (MESSAGE_TYPE));
        }
        //GETTING FILE MESSAGE (35 PTS)
        else if(fdef){
            /*
            Request an entire file by first sending afile messag eto get ts lenght and 
            then a series of file messages to get the actulal content of the file. Put 
            recieved file under the "recieved/" directory with the same name as the 
            orginal file. Compare the file agianst the origanl using linux command diff 
            and demonstrate that they are the exact same. Measure the file transfer
            */
            FIFORequestChannel chan ("control", FIFORequestChannel::CLIENT_SIDE);
            cout << "FILENAME = " << filename << endl;
            struct timeval start, end;
            gettimeofday( &start, NULL);
            filemsg *msg = new filemsg(0,0);
            int request = sizeof(filemsg) + filename.size() + 1;
            char *buf = new char[request];
            memcpy(buf, msg, sizeof(filemsg));
            strcpy(buf + sizeof(filemsg), filename.c_str());
            chan.cwrite(buf, request);

            __int64_t filesize;
            chan.cread(&filesize, sizeof(__int64_t));
            
            string output_path = string("received/") + filename;
            FILE *f = fopen(output_path.c_str() , "wb");
            char *reciever = new char[MAX_MESSAGE];

            while( filesize > 0){
                int r_lenth = min((__int64_t)MAX_MESSAGE, filesize);
                ((filemsg*)buf)->length = r_lenth;
                chan.cwrite(buf, request);
                chan.cread(reciever, r_lenth);
                fwrite(reciever, 1, r_lenth, f);
                ((filemsg *)buf)->offset += r_lenth;
                filesize -= r_lenth;
            }
            fclose(f);
            gettimeofday( &end, NULL);
            double time_taken = (end.tv_usec - start.tv_usec);
            cout << "Time_taken to get the file : " << time_taken << endl;
            // closing the channel  (5 PTS)  
            MESSAGE_TYPE m = QUIT_MSG;
            chan.cwrite (&m, sizeof (MESSAGE_TYPE));
        }
        //REQUESTING A NEW CHANNEL (15 PTS)
        else if (cdef){
            /*
            Ask the server to create a new channel for you tby sending a special NEWCHANNEL_MSG 
            request and join that channel. After the channel is created, demonstrated that you 
            can use that to speak to the server. Sending a few data points requests and recieving 
            their responces is adequte for that demonstration.
            */
            FIFORequestChannel chan ("control", FIFORequestChannel::CLIENT_SIDE);
            MESSAGE_TYPE new_channel = NEWCHANNEL_MSG;
            chan.cwrite(&new_channel, sizeof(new_channel));
            char new_channel_name [30];
            chan.cread(&new_channel_name , sizeof(new_channel_name) );
            //cout << new_channel_name << endl;

            FIFORequestChannel new_chan (new_channel_name, FIFORequestChannel::CLIENT_SIDE);
            
            //using the new channel to get a data point
                datamsg *data = new datamsg( 1, 1, 1);
                new_chan.cwrite(data, sizeof(datamsg));
                double recived_val = 0;
                new_chan.cread( &recived_val, sizeof(double));
                cout << recived_val << endl;
                
            // closing the channels  (5 PTS)  
            MESSAGE_TYPE m = QUIT_MSG;
            new_chan.cwrite (&m, sizeof (MESSAGE_TYPE));
            chan.cwrite (&m, sizeof (MESSAGE_TYPE));
        }
        
    }
    else{ 
        cout << "Running server as a child process pid = " << pid << endl;
        char* args[] = {"./server", NULL};
        //cout << args[0] <<endl;
        execv(args[0], args);
        
        
    }
}
  
