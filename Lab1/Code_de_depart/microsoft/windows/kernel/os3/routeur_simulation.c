
/*
*********************************************************************************************************
*                                                 uC/OS-III
*                                          The Real-Time Kernel
*                                               PORT Windows
*
*
*            		          					Guy BOIS
*                                  Polytechnique Montreal, Qc, CANADA
*                                                  02/2021
*
* File : routeur.c
*
*********************************************************************************************************
*/

#include "routeur.h"

#include  <cpu.h>
#include  <lib_mem.h>
#include  <os.h>
#include  "os_app_hooks.h"
#include  "app_cfg.h"

// À utiliser pour suivre le remplissage et le vidage des fifos
// Mettre en commentaire et utiliser la fonction vide suivante si vous ne voulez pas de trace
#define safeprintf(fmt, ...)															\
{																						\
	OSMutexPend(&mutPrint, 0, OS_OPT_PEND_BLOCKING, &ts, &perr);						\
	printf(fmt, ##__VA_ARGS__);															\
	OSMutexPost(&mutPrint, OS_OPT_POST_NONE, &perr);									\
}

// À utiliser pour ne pas avoir les traces de remplissage et de vidage des fifos
//#define safeprintf(fmt, ...)															\
//{			}



///////////////////////////////////////////////////////////////////////////////////////
//								Routines d'interruptions
///////////////////////////////////////////////////////////////////////////////////////

/*
À venir dans la partie 2
*/



/*
*********************************************************************************************************
*                                                  MAIN
*********************************************************************************************************
*/

int main(void)
{

    OS_ERR  os_err;

	CPU_IntInit();

	Mem_Init();                                                 // Initialize Memory Managment Module                   
	CPU_IntDis();                                               // Disable all Interrupts                               
	CPU_Init();                                                 // Initialize the uC/CPU services                       

	OSInit(&os_err);

	create_application();

    OSStart(&os_err);

    return 0;
}

void create_application() {
	int error;

	error = create_events();
	if (error != 0)
		printf("Error %d while creating events\n", error);

	error = create_tasks();
	if (error != 0)
		printf("Error %d while creating tasks\n", error);

}

int create_tasks() {
	
	int i;
	
	for(i = 0; i < NB_OUTPUT_PORTS; i++)
	{
		Port[i].id = i;
		switch(i)
		{
			case 0:
				Port[i].name = "Port 0";
				break;
			case 1:
				Port[i].name = "Port 1";
				break;
			case 2:
				Port[i].name = "Port 2";
				break;
			default:
				break;
		};
	}

	// Creation des taches
	OS_ERR err;

	OSTaskCreate(&TaskGenerateTCB, 		"TaskGenerate", 	TaskGenerate,	(void*)0, 	TaskGeneratePRIO, 	&TaskGenerateSTK[0u], 	TASK_STK_SIZE / 2, TASK_STK_SIZE, 1, 0, (void*) 0,(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), &err );
	
	OSTaskCreate(&TaskComputingTCB, 	"TaskComputing", 	TaskComputing, 	(void*)0, 	TaskComputingPRIO, 	&TaskComputingSTK[0u], 	TASK_STK_SIZE / 2, TASK_STK_SIZE, 1024, 0, (void*) 0,(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), &err );

	OSTaskCreate(&TaskForwardingTCB,	"TaskForwarding", 	TaskForwarding,	(void*)0, 	TaskForwardingPRIO, &TaskForwardingSTK[0u], TASK_STK_SIZE / 2, TASK_STK_SIZE, 1, 0, (void*) 0,(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), &err );

// Pour éviter d'avoir 3 fois le même code on a un tableau pour lequel chaque entrée appel TaskOutputPort avec des paramètres différents
	for(i = 0; i < NB_OUTPUT_PORTS; i++){
	OSTaskCreate(&TaskOutputPortTCB[i], "OutputPort", 	TaskOutputPort, &TaskOutputPortTCB[i], TaskOutputPortPRIO, &TaskOutputPortSTK[i][0u], TASK_STK_SIZE / 2, TASK_STK_SIZE, 1, 0, (void*) 0,(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), &err );
	};

	OSTaskCreate(&TaskStatsTCB, "TaskStats", TaskStats, (void*)0, TaskStatsPRIO, &TaskStatsSTK[0u], TASK_STK_SIZE / 2, TASK_STK_SIZE, 1, 0, (void*)0, (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), &err);

	return 0;
}

int create_events() {
	OS_ERR err;
	int i;

	// Creation des semaphores

	OS_EVENT* semStop;
	OS_EVENT* semReset;
	OS_EVENT* semStat;
	// Pas de sémaphore pour la partie 1
	
	// Creation des mutex
	OSMutexCreate(&mutRejete, "mutRejete", &err);
	OSMutexCreate(&mutPrint, "mutPrint", &err);
	OSMutexCreate(&mutAlloc, "mutAlloc", &err);

	// Creation des files externes  - vous pourrez diminuer au besoin la longueur des files
	OSQCreate(&lowQ, "lowQ", 1024, &err);
	OSQCreate(&mediumQ, "mediumQ", 1024, &err);
	OSQCreate(&highQ, "highQ", 1024, &err);
	

	return 0;
}


///////////////////////////////////////////////////////////////////////////////////////
//									TASKS
///////////////////////////////////////////////////////////////////////////////////////

/*
 *********************************************************************************************************
 *											  TaskGeneratePacket
 *  - Génère des paquets et les envoie dans le fifo d'entrée.
 *  - À des fins de développement de votre application, vous pouvez *temporairement* modifier la variable
 *    "shouldSlowthingsDown" à true pour ne générer que quelques paquets par seconde, et ainsi pouvoir
 *    déboguer le flot de vos paquets de manière plus saine d'esprit. Cependant, la correction sera effectuée
 *    avec cette variable à false.
 *********************************************************************************************************
 */
void TaskGenerate(void *data) {
	srand(42);
	OS_ERR err, perr;
	CPU_TS ts;
	bool isGenPhase = false; 		//Indique si on est dans la phase de generation ou non
	const bool shouldSlowThingsDown = false;		//Variable à modifier
	int packGenQty = (rand() % 250);
	while(true) {
		if (isGenPhase) {
			OSMutexPend(&mutAlloc, 0, OS_OPT_PEND_BLOCKING, &ts, &err);
				Packet *packet = malloc(sizeof(Packet));
			OSMutexPost(&mutAlloc, OS_OPT_POST_NONE, &err);

			packet->src = rand() * (UINT32_MAX/RAND_MAX);
			packet->dst = rand() * (UINT32_MAX/RAND_MAX);
			packet->type = rand() % NB_PACKET_TYPE;

			for (int i = 0; i < ARRAY_SIZE(packet->data); ++i)
				packet->data[i] = (unsigned int)rand();
			packet->data[0] = nbPacketCrees;

			nbPacketCrees++;
			OSMutexPend(&mutPrint, 0, OS_OPT_PEND_BLOCKING, &ts, &err);
			//if (shouldSlowThingsDown) {
				printf("GENERATE : ********Generation du Paquet # %d ******** \n", nbPacketCrees);
				printf("ADD %x \n", packet);
				printf("	** src : %x \n", packet->src);
				printf("	** dst : %x \n", packet->dst);
				printf("	** type : %d \n", packet->type);
			OSMutexPost(&mutPrint, OS_OPT_POST_NONE, &err);
			
			//}

			OSTaskQPost(&TaskComputingTCB, packet, sizeof(packet), OS_OPT_POST_FIFO + OS_OPT_POST_NO_SCHED, &err);

			safeprintf("Nb de paquets dans le fifo d'entrée - apres production de TaskGenenerate: %d \n", TaskComputingTCB.MsgQ.NbrEntries);

			if (err == OS_ERR_Q_MAX) {
				
				OSMutexPend(&mutAlloc, 0, OS_OPT_PEND_BLOCKING, &ts, &err);
				safeprintf("GENERATE: Paquet rejete a l'entree car la FIFO est pleine !\n");
					free(packet);
				OSMutexPost(&mutAlloc, OS_OPT_POST_NONE, &err); 
				packet_rejete_fifo_pleine_inputQ++;
			} 

			if (shouldSlowThingsDown) {
				OSTimeDlyHMSM(0,0,0, 200 + rand() % 600, OS_OPT_TIME_HMSM_STRICT, &err);
			} else {
				OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_TIME_HMSM_STRICT, &err);

				if ((nbPacketCrees % packGenQty) == 0) //On genère jusqu'à 250 paquets par phase de géneration
				{
					isGenPhase = false;
				}
			}
		}
		else
		{
			OSTimeDlyHMSM(0, 0, 2, 0, OS_OPT_TIME_HMSM_STRICT, &err);
			isGenPhase = true;
			do { packGenQty = (rand() % 256); } while (packGenQty == 0);
			
			safeprintf("GENERATE: Generation de %d paquets durant les %d prochaines millisecondes\n", packGenQty, packGenQty*2);
		}
	}
}

/*
 *********************************************************************************************************
 *											  TaskStop
 *  -Stoppe le routeur une fois que 100 paquets ont été rejetés pour mauvais CRC
 *  -Ne doit pas stopper la tâche d'affichage des statistiques.
 *********************************************************************************************************
 */
// Partie 2 (oubliez ça pour l'instant)



/*
 *********************************************************************************************************
 *											  TaskReset
 *  -Remet le compteur de mauvaises sources à 0
 *  -Si le routeur était arrêté, le redémarre
 *********************************************************************************************************
 */
// Partie 2 (oubliez ça pour l'instant)

/*
 *********************************************************************************************************
 *											  TaskComputing
 *  -Vérifie si les paquets sont conformes (CRC,Adresse Source)
 *  -Dispatche les paquets dans des files (HIGH,MEDIUM,LOW)
 *
 *********************************************************************************************************
 */
void TaskComputing(void *pdata) {
	OS_ERR err, perr;
	CPU_TS ts;
	OS_MSG_SIZE msg_size;
	Packet *packet = NULL;
	OS_TICK actualticks = 0;
	while(true){

//		1) Appel de fonction à compléter, 2) compléter safeprint et 3) compléter err_msg 
//		safeprintf("Nb de paquets dans le fifo d'entrée - apres consommation de TaskComputing: %d \n", À compléter);
//		err_msg("À compléter",err);

		/* On simule un temps de traitement avec ce compteur bidon.
		 * Cette boucle devrait prendre entre 2 et 4 ticks d'OS (considérez
		 * exactement 3 ticks pour la question dans l'énoncé).
		 */
//			Code de l'attente active à compléter, utilisez la constante WAITFORComputing 
		
		OSTaskQPost(&TaskComputingTCB, packet, sizeof(packet), OS_OPT_POST_FIFO + OS_OPT_POST_NO_SCHED, &err);
		safeprintf("Nb de paquets dans le fifo d'entrée - apres consommation de TaskComputing: %d \n", TaskComputingTCB.MsgQ.Nb;
		err_msg("Task Computing Flop", err);


		//Verification de l'espace d'addressage
		if ((packet->src > REJECT_LOW1 && packet->src < REJECT_HIGH1) ||
			(packet->src > REJECT_LOW2 && packet->src < REJECT_HIGH2) ||
			(packet->src > REJECT_LOW3 && packet->src < REJECT_HIGH3) ||
			(packet->src > REJECT_LOW4 && packet->src < REJECT_HIGH4)) {
			OSMutexPend(&mutRejete, 0, OS_OPT_PEND_BLOCKING, &ts, &err);
				++nbPacketSourceRejete;

			OSMutexPend(&mutPrint, 0, OS_OPT_PEND_BLOCKING, &ts, &err);
			printf("\n--TaskComputing: Source invalide (Paquet rejete) (total : %d)\n", nbPacketSourceRejete);
			printf("\n--Il s agit du paquet\n");
			printf("	** src : %x \n", packet->src);
			printf("	** dst : %x \n", packet->dst);
			OSMutexPost(&mutPrint, OS_OPT_POST_NONE, &err);

			OSMutexPost(&mutRejete, OS_OPT_POST_NONE, &err);

			OSMutexPend(&mutAlloc, 0, OS_OPT_PEND_BLOCKING, &ts, &err);
					free(packet);
			OSMutexPost(&mutAlloc, OS_OPT_POST_NONE, &err); 
		} else {

			//Dispatche les paquets selon leur type
			switch (packet->type) {
			case PACKET_VIDEO:
//			1) Appel de fonction à compléter et 2) compléter safeprint
//			safeprintf("Nb de paquets dans ... - apres production de TaskComputing: %d \n", À compléter);
				
				err = OSQPost(highQ, packet);
				safeprintf("Nb de paquets dans ... - apres production de TaskComputing: %d \n", err);
			break;

			case PACKET_AUDIO:
//			1) Appel de fonction à compléter et 2) compléter safeprint
//			safeprintf("Nb de paquets dans ... - apres production de TaskComputing: %d \n", À compléter);
				err = OSQPost(mediumQ, packet);
			break;

			case PACKET_AUTRE:
//			1) Appel de fonction à compléter et 2) compléter safeprint
//			safeprintf("Nb de paquets dans ... - apres production de TaskComputing: %d \n", À compléter);
				err = OSQPost(lowQ, packet);
			break;

			default:
				OSMutexPend(os_mutex_data, 0, err);
				free(packet);
				OSMutexPost(os_mutex_data);
				break;
			}
			if (err == OS_ERR_Q_MAX)
				safeprintf("TaskComputing : QFULL.\n");
			
		}
	}
}


/*
 *********************************************************************************************************
 *											  TaskForwarding
 *  -traite la priorité des paquets : si un paquet de haute priorité est prêt,
 *   on l'envoie à l'aide de la fonction dispatch_packet, sinon on regarde les paquets de moins haute priorité
 *********************************************************************************************************
 */
void TaskForwarding(void *pdata) {
	OS_ERR perr, err = OS_ERR_NONE;
	CPU_TS ts;
	OS_MSG_SIZE msg_size;
	Packet *packet = NULL;

	while(1){
		/* Si paquet vidéo prêt */
//		1) Appel de fonction à compléter et 2) compléter safeprint
//		safeprintf("Nb de paquets dans ... - apres consommation de TaskFowarding: %d \n", À compléter);
		if(err == OS_ERR_NONE){
			/* Envoi du paquet */
			dispatch_packet(packet);
			safeprintf("\n--TaskForwarding:  paquets %d envoyes\n\n", ++nbPacketTraites);
		}else{
			/* Si paquet audio prêt */
//			1) Appel de fonction à compléter et 2) compléter safeprint
//			safeprintf("Nb de paquets dans ... - apres consommation de TaskFowarding: %d \n", À compléter);
			if(err == OS_ERR_NONE){
				/* Envoi du paquet */
				dispatch_packet(packet);
				safeprintf("\n--TaskForwarding: paquets %d envoyes\n\n", ++nbPacketTraites);
			}else{
				/* Si paquet autre prêt */
//				1) Appel de fonction à compléter et 2) compléter safeprint				
//				safeprintf("Nb de paquets dans ... - apres consommation de TaskFowarding: %d \n", À compléter);
				if(err == OS_ERR_NONE){
					/* Envoi du paquet */
					dispatch_packet(packet);
					safeprintf("\n--TaskForwarding: paquets %d envoyes\n\n", ++nbPacketTraites);
				}
			}
		}
	}
}

/*
 *********************************************************************************************************
 *											  Fonction Dispatch
 *  -Envoie le paquet passé en paramètre vers la mailbox correspondante à son adressage destination
 *********************************************************************************************************
 */
 void dispatch_packet (Packet* packet){
	OS_ERR err, perr;
	CPU_TS ts;
	OS_MSG_SIZE msg_size;

	/* Test sur la destination du paquet */
	if(packet->dst >= INT1_LOW && packet->dst <= INT1_HIGH ){

		safeprintf("\n--Paquet dans Output Port no 0\n");
		err = OSMboxPost(mbox[0], p);
//		Appel de fonction à compléter
	}
	else {
			if (packet->dst >= INT2_LOW && packet->dst <= INT2_HIGH ){
			safeprintf("\n--Paquet dans Output Port no 1\n");
			err = OSMboxPost(mbox[1], p);
//			Appel de fonction à compléter
			}
			else {
				if(packet->dst >= INT3_LOW && packet->dst <= INT3_HIGH ){
					safeprintf("\n--Paquet dans OutputPort no 2\n");
//					Appel de fonction à compléter
					err = OSMboxPost(mbox[2], p);
					}
					else {
						if(packet->dst >= INT_BC_LOW && packet->dst <= INT_BC_HIGH){
						Packet* others[2];
						int i;
						for (i = 0; i < ARRAY_SIZE(others); ++i) {
							OSMutexPend(&mutAlloc, 0, OS_OPT_PEND_BLOCKING, &ts, &err);
							others[i] = malloc(sizeof(Packet));
							OSMutexPost(&mutAlloc, OS_OPT_POST_NONE, &err);
							memcpy(others[i], packet, sizeof(Packet));
							}
						safeprintf("\n--Paquet BC dans Output Port no 0 à 2\n");
//						Appels de fonction à compléter
						err=OSMboxPost(mbox)
						}
					}
			}
	}
	if(err == OS_ERR_Q_MAX){
		/*Destruction du paquet si la mailbox de destination est pleine*/
		
		OSMutexPend(&mutAlloc, 0, OS_OPT_PEND_BLOCKING, &ts, &err);
			safeprintf("\n--TaskForwarding: Erreur mailbox full\n");
			free(packet);
			packet_rejete_output_port_plein++;
		OSMutexPost(&mutAlloc, OS_OPT_POST_NONE, &err); 
		
	}
 }

/*
 *********************************************************************************************************
 *											  TaskPrint
 *  -Affiche les infos des paquets arrivés à destination et libere la mémoire allouée
 *********************************************************************************************************
 */
void TaskOutputPort(void *data) {
	OS_ERR err, perr;
	CPU_TS ts;
	OS_MSG_SIZE msg_size;
	Packet* packet = NULL;
	Info_Port info = *(Info_Port*)data;

	while(1){
		/*Attente d'un paquet*/
//		1) Appel de fonction à compléter, 2) compléter err_msg 
//		err_msg("PRINT : À compléter",err);

		OSMutexPend(&mutPrint, 0, OS_OPT_PEND_BLOCKING, &ts, &err);
		/*impression des infos du paquets*/
		printf("\nPaquet recu en %d \n", info.id);
		printf("    >> src : %x \n", packet->src);
		printf("    >> dst : %x \n", packet->dst);
		printf("    >> type : %d \n", packet->type);
		OSMutexPost(&mutPrint, OS_OPT_POST_NONE, &err);

		/*Libération de la mémoire*/
		OSMutexPend(&mutAlloc, 0, OS_OPT_PEND_BLOCKING, &ts, &err);
			free(packet);
		OSMutexPost(&mutAlloc, OS_OPT_POST_NONE, &err);
	}

}

/*
 *********************************************************************************************************
 *                                              TaskStats
 *  -Est déclenchée lorsque le gpio_isr() libère le sémaphore
 *  -Lorsque déclenchée, imprime les statistiques du routeur à cet instant
 *********************************************************************************************************
 */
void TaskStats(void* pdata) {
	OS_ERR err, perr;
	CPU_TS ts;
	OS_TICK actualticks;

	OSStatTaskCPUUsageInit(&err);
	Suspend_Delay_Resume_All(1);
	OSStatReset(&err);

	while (1) {


		OSMutexPend(&mutPrint, 0, OS_OPT_PEND_BLOCKING, &ts, &err);

		printf("\n------------------ Affichage des statistiques ------------------\n\n");

//		À compléter en utilisant la numérotation de 1 à 15  dans l'énoncé du laboratoire
		xil_printf("1 : Nb de paquets total créés: %d\n", nbPacketCrees);
		xil_printf("2: Nb de paquets total traités: %d\n", nbPacketTraites);
		xil_printf("Nb de paquets rejetés pour mauvaise source (adresse)  %d\n", nbPacketTraites);

		OSMutexPost(&mutPrint, OS_OPT_POST_NONE, &err);

		Suspend_Delay_Resume_All(10);

		OSTimeDlyHMSM(0, 0, 10, 0, OS_OPT_TIME_HMSM_STRICT, &err);
	}
}

/*
 *********************************************************************************************************
 *                                              Suspend_Delay_Resume
 *  -Utilise lors de l'initialisation de la tache statistique
 *  -Permet aussi d'arrêter l'exécution durant l'exécution
 *********************************************************************************************************
 */

void Suspend_Delay_Resume_All(int nb_sec) {

OS_ERR err;
int i;

	OSTaskSuspend(&TaskGenerateTCB, &err);
	OSTaskSuspend(&TaskComputingTCB, &err);
	OSTaskSuspend(&TaskForwardingTCB, &err);

	for (i = 0; i < NB_OUTPUT_PORTS; i++) {
		OSTaskSuspend(&TaskOutputPortTCB[i], &err);
	};

	OSTimeDlyHMSM(0, 0, nb_sec, 0, OS_OPT_TIME_HMSM_STRICT, &err);

	OSTaskResume(&TaskGenerateTCB, &err);
	OSTaskResume(&TaskComputingTCB, &err);
	OSTaskResume(&TaskForwardingTCB, &err);
	for (i = 0; i < NB_OUTPUT_PORTS; i++) {
		OSTaskResume(&TaskOutputPortTCB[i], &err);

	}

}


void err_msg(char* entete, uint8_t err)
{
	if(err != 0)
	{
		printf(entete);
		printf(": Une erreur est retournée : code %d \n",err);
	}
}
