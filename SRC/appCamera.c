/*
 ============================================================================
 Name        : appCamera.c
 Author      :
 Version     :
 Copyright   :
 Description : Test application for usb camera device
 ============================================================================
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/sched.h>

#include "cameraUsbDriver.h"
#include "dht_data.h"

#define USB_CHAR_CAMERA_0 "/dev/cameraEle784num0"


void clrBuffer(void) {
    int c;
    while ((c=getchar()) != '\n' && c != EOF)
        ;
}

void clrTerminal(void) {
     printf("\033[H\033[J"); //Clear terminal screen
}

//Open and return the file descriptor
int openDriverRead(char blocking){

	 int devFd =  0; //File descriptor for the char device
	 if(blocking==1){ //Blocking
		  printf("Camera Char Device opening : %s| O_RDONLY \n",USB_CHAR_CAMERA_0);
          devFd = open(USB_CHAR_CAMERA_0, O_RDONLY);
    }
    else{ //Non Blocking
        printf("Device opening : %s| O_RDONLY|O_NONBLOCK \n",USB_CHAR_CAMERA_0);
        devFd = open(USB_CHAR_CAMERA_0, (O_RDONLY | O_NONBLOCK));

    }

	 if(devFd >= 0){
		 	printf("Camera Char Device is open\n");
	 }
	 else{
			printf("O_RDONLY | Error : Camera Char Device is not open : error code = %d  \n",devFd);
            printf(" Press a key to continue \n");
            getchar();
	 }
	 return devFd;
}


//Open driver in Write Mode and return the file descriptor
int openDriverWrite(char blocking){

	 int devFd =  0; //File descriptor for the char device
	 if(blocking){ //Blocking
			printf("Camera Char Device opening : %s| O_WRONLY \n",USB_CHAR_CAMERA_0);
			devFd = open(USB_CHAR_CAMERA_0, O_WRONLY);
    }
    else{   //NonBlocking
        printf("Camera Char Device opening : %s| O_WRONLY | O_NONBLOCK \n",USB_CHAR_CAMERA_0);
		  devFd = open(USB_CHAR_CAMERA_0, O_WRONLY | O_NONBLOCK);
    }

	 if(devFd >= 0){
		 	printf("Camera Char Device is open\n");
	 }
	 else{
			printf("O_WRONLY | Error : Camera Char Device is not open\n");
            printf(" Press a key to continue \n");
            getchar();
	 }
	 return devFd;
}

//Open driver in Read/Write Mode and return the file descriptor
int openDriverReadWrite(char blocking){

	 int devFd =  0; //File descriptor for the char device
	 if(blocking){ //Blocking
			printf("Camera Char Device opening : %s| O_RDWR \n",USB_CHAR_CAMERA_0);
			devFd = open(USB_CHAR_CAMERA_0, O_RDWR);
    }
    else{   //NonBlocking
            printf("Camera Char Device opening : %s| O_RDWR \n",USB_CHAR_CAMERA_0);
		    devFd = open(USB_CHAR_CAMERA_0, O_RDWR | O_NONBLOCK);
    }

	 if(devFd >= 0){
		 	printf("Camera Char Device is open\n");
	 }
	 else{
			printf("O_RDWR | Error : Camera Char Device is not open\n");
            printf(" Press a key to continue \n");
            getchar();
	 }
	 return devFd;
}


void readFunction(int devFd,char blocking) {

    int i=0;
    int status = 0;
    unsigned int nbCharToRead = 0;
	 char *bufRead;
	 char usrCharBuf[256];


     printf("|Read Buffer|_ Type the number of characters to read (1 to 256):\n");
     scanf("%d", &nbCharToRead);
     clrBuffer();

     //Nombre incorrecte de caractère à lire
     if ((nbCharToRead>256)||(nbCharToRead<=0)){
         printf("|Read Buffer|_ Incorrect Number of character to read (must be <256 & >0 \n");
     }
     //Lecture
     else{

         printf("|Read Buffer|_ Reading %d character(s) \n",nbCharToRead);
         status = read(devFd, &usrCharBuf, nbCharToRead);
			printf("|Read Buffer|_ read() function return status = %d  \n",status);
			// if ((status == -EAGAIN)&&(blocking==0)){
         if (status == EAGAIN){
             printf("|Read Buffer|_ EAGAIN, Error : Buffer is empty (NonBlocking read)\n");
         }
         //else if (status == ERESTARTSYS){
         //    printf("|Read Buffer|_ ERESTARTSYS, Blocking read : Error in wait_event_interruptible when read fall asleep\n");
         //}
         else if (status < 0){
             printf("|Read Buffer|_ Error Read return an unknown negative value\n");
         }
         else if (status >= 0){
             printf("|Read Buffer|_ Number of read character(s) = %d \n", status);
             printf("|Read Buffer|_ read character(s) : ");
             for(i=0;i<nbCharToRead;i++){ //show read characters
                 printf("%c",usrCharBuf[i]);
             }
				 printf("\n");
         }
     }

}


long ioctlFunction(int fd, int cmd, unsigned long arg){
    long retval = 0;
    switch(cmd){
        case IOCTL_GET :   retval = ioctl(fd,IOCTL_GET,arg);
                   break;
        case IOCTL_SET :    retval = ioctl(fd,IOCTL_SET,arg);
                    break;
        case IOCTL_STREAMON :    retval = ioctl(fd,IOCTL_STREAMON,arg);
                                 if(retval >= 0){
                                    printf("IOCTL_STREAMON SUCCESS\n");
                                 }
                                 else{
                                    printf("IOCTL_STREAMON FAIL %ld\n",retval);
                                 }
                                 break;
        case IOCTL_STREAMOFF :    retval = ioctl(fd,IOCTL_STREAMOFF,arg);
                                  if(retval >= 0){
                                    printf("IOCTL_STREAMOFF SUCCESS\n");
                                 }
                                 else{
                                    printf("IOCTL_STREAMOFF FAIL %ld\n",retval);
                                 }
                                 break;
                    break;
        case IOCTL_GRAB :    retval = ioctl(fd,IOCTL_GRAB,arg);
                             if(retval >= 0){
                                    printf("IOCTL_GRAB SUCCESS\n");
                                 }
                                 else{
                                    printf("IOCTL_GRAB FAIL %ld\n",retval);
                                 }
                                 break;
                    break;
        case IOCTL_PANTILT :    retval = ioctl(fd,IOCTL_PANTILT,arg);
                    break;
        case IOCTL_PANTILT_RESEST :    retval = ioctl(fd,IOCTL_PANTILT_RESEST,arg);
                    break;

        default : return -1;
    }
    return retval;
}



void writeFunction(int devFd,char blocking) {

    int i=0;
    int status = 0;
    char usrChar = 0;
    int  nbCharToWrite = 0;
    char usrCharBuf[256];


		printf("|Write Buffer|_ Type characters to write in the buffer [Enter to finish]:\n");
		scanf("%s", &usrCharBuf);
		//usrChar = getchar();
		printf("chaine tapée : %s\n",usrCharBuf);
		nbCharToWrite = strlen(usrCharBuf);
		printf("nombre de caractère : %d\n",nbCharToWrite);
		clrBuffer();

	  //Nombre incorrecte de caractère à écrire
	  if ((nbCharToWrite>256)||(nbCharToWrite<=0)){
		  //Aucun caractère à écrire
		  if (nbCharToWrite==0){
		      printf("|Write Buffer|_ 0 character to write: %d \n",nbCharToWrite);
		  }
		  else{
		  		printf("|Write Buffer|_ Incorrect Number of character to write (must be <256 & >0 \n");
		  }
	  }
	  //Ecriture
	  else{
	      //bufWrite = malloc(nbCharToWrite);
	      //memset(bufWrite,0,nbCharToWrite); //initialize a memory buffer of size nbCharToRead

	      printf("|Write Buffer|_ Writing %d character(s) \n",nbCharToWrite);
	      //status = write(devFd, &usrCharBuf, nbCharToWrite);
			status = write(devFd, &usrCharBuf, nbCharToWrite);
			printf("|Write Buffer|_ write() function return status = %d  \n",status);
	      if ((status == -EAGAIN)&&(blocking==0)){
	          printf("|Write Buffer|_ EAGAIN, Error : Buffer is full (NonBlocking Write)\n");
	      }

	      else if (status < 0){
	          printf("|Write Buffer|_ Error Read return an unknown negative value\n");
	      }
	      else if (status >= 0){
               printf("|Write Buffer|_ %d characters were written successfully\n",status);
	          //printf("|Write Buffer|_ %d characters were written successfully\n",nbCharToWrite);
	      }
	      //free(bufWrite); //free memory buffer
	  }

}


int main(void) {
		int i = 0;
		unsigned int nbCharToWrite = 0;
		char usrChar = 0;
		int devFd =  0; //File descriptor for the char device
		int status = 0;
        unsigned long bufferSize = 0;

		char userChoice = '0';
		char blocking = '0'; //0=nonBlocking 1=Blocking

		unsigned int incorrectKeyCounter = 0; //While loop security

		/* Variable pour la prise de photos */
		FILE *foutput;
        unsigned char * inBuffer;
        unsigned char * finalBuf;
        long mySize;

        while (userChoice!='5'){

            printf("----------------------------------\n");
            printf("---------Camera Application-------\n");
            printf("----------------------------------\n \n");
            printf("MENU :\n ----- \n 1) Take a picture \n 2) IOCTL Commands\n \n 5) Exit\n-> ");
            scanf("%c", &userChoice);
            clrBuffer();

            switch (userChoice)
            {

                case '1':  //Take a picture
                    clrTerminal();
                    printf("------------Take a Picture-----------\n");
                    printf("----------------------------------\n \n");

                    blocking = 1; //NonBlocking
                    devFd = openDriverRead(blocking); //Open driver in Read Mode

                    if(devFd>0){

                        //inBuffer = malloc((42666)* sizeof(unsigned char));
                        //finalBuf = malloc((42666 * 2)* sizeof(unsigned char));
                        inBuffer = (unsigned char*)malloc(42666);
                        finalBuf = (unsigned char*)malloc(42666 * 2);
                        if((inBuffer == NULL) || (finalBuf == NULL)){
                            printf("Erreur malloc inBuffer ou finalBuf");
                            return -1;
                        }

                        //Etape #1
                        foutput = fopen("/home/johann/Bureau/photoCamUsb.jpg", "wb");

                        if(foutput != NULL){
                            // Etape #2
                            printf("-------IOCTL_STREAMON--------\n");
                            status = ioctlFunction(devFd, IOCTL_STREAMON, 0);
                            if (status>=0){
                                printf("-IOCTL_STREAMON OK \n");
                            }
                            else{
                                printf("-IOCTL_STREAMON ERROR \n");
                            }

                            // Etape #3
                            printf("-------IOCTL_GRAB--------\n");
                            status = ioctlFunction(devFd, IOCTL_GRAB, 0 );
                            if (status>=0){
                                printf("-IOCTL_GRAB OK \n");
                            }
                            else{
                                printf("-IOCTL_GRAB ERROR \n");
                            }

                            // Etape #4
                            printf("----------READ------------\n");
                            mySize = read(devFd, inBuffer, 42666);
                            if (mySize>=0){
                                printf("-READ OK \n");
                            }
                            else{
                                printf("-READ ERROR \n");
                            }
                            // Etape #5
                            printf("-------IOCTL_STREAMOFF--------\n");
                            status = ioctlFunction(devFd, IOCTL_STREAMOFF, 0);
                            if (status>=0){
                                printf("-IOCTL_STREAMOFF OK \n");
                            }
                            else{
                                printf("-IOCTL_STREAMOFF ERROR \n");
                            }

                            //Etape #6
                            memcpy (finalBuf, inBuffer, HEADERFRAME1);
                            memcpy (finalBuf + HEADERFRAME1, dht_data, DHT_SIZE);
                            memcpy (finalBuf + HEADERFRAME1 + DHT_SIZE,
                                    inBuffer + HEADERFRAME1,
                                    (mySize  - HEADERFRAME1));
                            printf("memcopy passe \n");

                            //Etape #7
                            status = fwrite (finalBuf, mySize + DHT_SIZE, 1, foutput);
                            printf("fwrite status = %d \n",status);

                            //Etape #8
                            status = fclose(foutput);
                            printf("fclose status = %d \n",status);
                            free(inBuffer);
                            free(finalBuf);

                            while(userChoice!='q'){
                                printf("Photo enregistré dans ~/Bureau/photoCamUsb.jpg\n\n q pour quitter");
                                scanf("%c", &userChoice);
                                clrBuffer();
                            }
                        }
                        else{
                            while(userChoice!='q'){
                                printf("Erreur Photo non enregistré\n\n q pour quitter");
                                scanf("%c", &userChoice);
                                clrBuffer();
                            }
                        }

                        close(devFd);
                    }

                    break;


                case '2':  //Commandes IOCTL


                    clrTerminal();
                    printf("------------IOCTL Commands--------\n");
                    printf("----------------------------------\n \n");

                    blocking = 1; //NonBlocking
                    devFd = openDriverRead(blocking); //Open driver in Read Mode

                    if(devFd>0){
                        while(userChoice!='q'){
                            printf("    1) IOCTL_GET\n    2) IOCTL_SET\n    3) IOCTL_STREAMON\n    4) IOCTL_STREAMOFF\n    5) IOCTL_GRAB\n    6) IOCTL_PANTILT\n    7) IOCTL_PANTILT_RESEST\n\n  q)Back to menu\n-> ");
                            scanf("%c", &userChoice);
                            clrBuffer();
                            switch (userChoice){
                            case '1' :
                                 printf("-------IOCTL_GET-------\n");
                                 printf("--------NO FUNCTION YET----------- \n-> ");
                                 //ioctlFunction(devFd, IOCTL_GET, ARG )

                                 break;

                            case '2' :
                                printf("-------IOCTL_SET-------\n");
                                printf("--------NO FUNCTION YET----------- \n-> ");
                                //ioctlFunction(devFd, IOCTL_SET, ARG )

                                break;

                            case '3' :
                                printf("-------IOCTL_STREAMON--------\n");
                                printf("--------NO FUNCTION YET----------- \n-> ");
                                //ioctlFunction(devFd, IOCTL_STREAMON, ARG )

                                break;

                            case '4' :
                                printf("-------IOCTL_STREAMOFF--------\n");
                                printf("--------NO FUNCTION YET----------- \n-> ");
                                //ioctlFunction(devFd, IOCTL_STREAMOFF, ARG )

                                break;

                            case '5' :
                                printf("-------IOCTL_GRAB--------\n");
                                printf("--------NO FUNCTION YET----------- \n-> ");
                                //ioctlFunction(devFd, IOCTL_GRAB, ARG )

                                break;

                            case '6' :
                                printf("-------IOCTL_PANTILT--------\n");
                                printf("---Rotation de la camera--- \n-> ");

                                while(userChoice!='q'){
                                    printf("1) Haut 2) Bas 3) Gauche 4) Droite \n");
                                    scanf("%c", &userChoice);
                                    clrBuffer();
                                    switch (userChoice){
                                        case '1' :
                                            printf("---Mouvement : Haut----\n");
                                            ioctlFunction(devFd, IOCTL_PANTILT, HAUT);
                                            break;
                                        case '2' :
                                            printf("---Mouvement : Haut----\n");
                                            ioctlFunction(devFd, IOCTL_PANTILT, BAS);
                                            break;
                                        case '3' :
                                            printf("---Mouvement : Haut----\n");
                                            ioctlFunction(devFd, IOCTL_PANTILT, GAUCHE);
                                            break;
                                        case '4' :
                                            printf("---Mouvement : Haut----\n");
                                            ioctlFunction(devFd, IOCTL_PANTILT, DROITE);
                                            break;
                                        default :
                                            printf("---Pas de direction selectionnee----\n");
                                            break;
                                    }
                                }
                                break;

                            case '7' :
                                printf("-------IOCTL_PANTILT_RESEST--------\n");
                                printf("--------NO FUNCTION YET----------- \n-> ");
                                ioctlFunction(devFd, IOCTL_PANTILT_RESEST, 0 );

                                break;

                            }
                            printf("\n\n\n");
                        }
                        close(devFd);
                    }

                    break;


                case '5':  //Exit Application
                    printf("Return to the main menu \n \n \n");
                    break;

                default :
                    printf("Incorrect key\n");
                    incorrectKeyCounter ++;
                    break;

            }
            if ((userChoice=='5') || (incorrectKeyCounter >= 5)){
                userChoice = '5'; //choice : exit application
            }
            else{
                userChoice = '0'; //Reset user choice to continue
            }
            clrTerminal();

        }

        printf("Fermeture de l'application ... Goodbye \n\n");
        return EXIT_SUCCESS;
}

